#include "core/algorithms/cfd/cfdfinder/cfdfinder.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <ranges>
#include <unordered_set>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/model/hyfd/inductor.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/preprocessor.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/sampler.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/validator.h"
#include "core/algorithms/cfd/cfdfinder/model/strategy_factory.h"
#include "core/algorithms/cfd/cfdfinder/types/frontier.h"
#include "core/algorithms/cfd/cfdfinder/util/lhs_utils.h"
#include "core/algorithms/cfd/cfdfinder/util/violations_util.h"
#include "core/algorithms/fd/hycommon/util/pli_util.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/indices/option.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"

namespace algos::cfdfinder {

CFDFinder::CFDFinder() : Algorithm() {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable, kEqualNulls});
}

void CFDFinder::ResetState() {
    cfd_collection_.clear();
}

void CFDFinder::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kMaximumLhs, kLimitPliCache, kCfdExpansionStrategy, kCfdPruningStrategy,
                          kCfdResultStrategy, kThreads, kRhsIndices});
};

void CFDFinder::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto legacy_eq = [](Pruning pruning_strategy) { return pruning_strategy == Pruning::kLegacy; };
    auto support_independent_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == Pruning::kSupportIndependent;
    };
    auto partial_fd_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == Pruning::kPartialFd;
    };
    auto get_schema_cols = [this]() { return schema_->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    auto calculate_default_rhs_indices = []() -> config::IndicesType { return {}; };
    RegisterOption(config::IndicesOption{
            kRhsIndices, kDRhsIndices, config::IndicesOption::NormalizeIndices,
            calculate_default_rhs_indices, true}(&rhs_filter_, get_schema_cols));

    RegisterOption(Option{&min_confidence_, kCfdMinimumConfidence, kDCfdMinimumConfidence, 1.0});
    RegisterOption(Option{&min_support_, kCfdMinimumSupport, kDCfdMinimumSupport, 0.8});
    RegisterOption(Option{&max_g1_, kMaximumG1, kDMaximumG1, 0.1});
    RegisterOption(Option{&max_patterns_, kMaxPatterns, kDMaxPatterns, 1000u});
    RegisterOption(Option{&min_support_gain_, kMinSupportGain, kDMinSupportGain, 1.0});
    RegisterOption(
            Option{&max_level_support_drop_, kMaxLevelSupportDrop, kDMaxLevelSupportDrop, 1.0});
    RegisterOption(Option{&limit_pli_cache_, kLimitPliCache, kDLimitPliCache, 50000u});
    RegisterOption(Option{&expansion_strategy_, kCfdExpansionStrategy, kDCfdExpansionStrategy});
    RegisterOption(Option{&result_strategy_, kCfdResultStrategy, kDCfdResultStrategy});
    RegisterOption(
            Option{&pruning_strategy_, kCfdPruningStrategy, kDCfdPruningStrategy}
                    .SetConditionalOpts({{legacy_eq, {kCfdMinimumSupport, kCfdMinimumConfidence}},
                                         {support_independent_eq,
                                          {kMaxPatterns, kMinSupportGain, kMaxLevelSupportDrop,
                                           kCfdMinimumConfidence}},
                                         {partial_fd_eq, {kMaximumG1}}}));
}

void CFDFinder::LoadDataInternal() {
    schema_ = RelationalSchema::CreateFrom(*input_table_);
    relation_ = CFDFinderRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: CFD mining is meaningless.");
    }
}

void CFDFinder::ExecuteInternal() {
    std::tie(plis_, inverted_plis_, compressed_records_) = Preprocess(relation_.get());

    auto levels = GetLattice();
    auto inverted_cluster_maps = BuildEnrichedStructures();

    auto pruning_strategy = CreatePruningStrategy(
            pruning_strategy_,
            PruningStrategyConfig{.num_rows = (*compressed_records_).size(),
                                  .min_support = min_support_,
                                  .min_confidence = min_confidence_,
                                  .max_g1 = max_g1_,
                                  .max_patterns = max_patterns_,
                                  .min_support_gain = min_support_gain_,
                                  .max_level_support_drop = max_level_support_drop_},
            inverted_plis_);
    auto result_receiver = CreateResultStrategy(result_strategy_);
    auto expansion_strategy = CreateExpansionStrategy(expansion_strategy_, compressed_records_,
                                                      inverted_cluster_maps);

    PLICache pli_cache(limit_pli_cache_, schema_.get(), std::move(plis_));

    size_t num_results = 0;
    int height = levels.size();
    --height;
    while (height >= 0) {
        auto start_level_time = std::chrono::system_clock::now();
        Level& current_level = levels[height];
        while (!current_level.empty()) {
            Candidate candidate = current_level.extract(current_level.begin()).value();
            auto const lhs_pli = pli_cache.GetLhsPli(candidate.lhs_);
            auto const& inverted_pli_rhs = inverted_plis_->at(candidate.rhs_);

            pruning_strategy->StartNewTableau(candidate);
            auto pattern_tableau = GenerateTableau(candidate.lhs_, lhs_pli.get(), inverted_pli_rhs,
                                                   expansion_strategy, pruning_strategy);

            if (!pruning_strategy->ContinueGeneration(pattern_tableau)) {
                continue;
            }
            if (height > 0) {
                auto& target_level = levels[height - 1];
                for (auto&& subset : utils::GenerateLhsSubsets(candidate.lhs_)) {
                    target_level.emplace(std::move(subset), candidate.rhs_);
                }
            }
            ++num_results;
            result_receiver->ReceiveResult(std::move(candidate), std::move(pattern_tableau));
        }
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - start_level_time);
        LOG_INFO("Finished level {} ({}), {} results.", height, elapsed_seconds.count(),
                 num_results);
        --height;
    }

    RegisterResults(std::move(result_receiver), std::move(inverted_cluster_maps));
    LOG_INFO("Total PLI computations: {}",
             pli_cache.GetTotalMisses() + pli_cache.GetFullHits() + pli_cache.GetPartialHits());
    LOG_INFO("Total PLI cache misses: {}", pli_cache.GetTotalMisses());
    LOG_INFO("Total PLI cache hits: {}", pli_cache.GetFullHits() + pli_cache.GetPartialHits());
    LOG_INFO("Total full PLI cache hits: {}", pli_cache.GetFullHits());
    LOG_INFO("Total partial PLI cache hits: {}", pli_cache.GetPartialHits());
}

InvertedClusterMaps CFDFinder::BuildEnrichedStructures() const {
    auto const& cluster_maps = relation_->GetClusterMaps();

    EnrichedPLIs enriched_plis;
    InvertedClusterMaps inverted_cluster_maps;
    enriched_plis.reserve(schema_->GetNumColumns());
    inverted_cluster_maps.reserve(schema_->GetNumColumns());

    for (size_t i = 0; i < schema_->GetNumColumns(); ++i) {
        auto enriched_pli = EnrichPLI(plis_->at(i), (*compressed_records_).size());
        InvertedClusterMap inverted_cluster_map;

        for (size_t cluster_id = 0; cluster_id < enriched_pli.size(); ++cluster_id) {
            int first_element = enriched_pli[cluster_id][0];
            for (auto const& [key, cluster] : cluster_maps[i]) {
                if (std::find(cluster.begin(), cluster.end(), first_element) != cluster.end()) {
                    inverted_cluster_map[cluster_id] = key;
                    break;
                }
            }
        }

        enriched_plis.push_back(std::move(enriched_pli));
        inverted_cluster_maps.push_back(std::move(inverted_cluster_map));
    }

    EnrichCompressedRecords(std::move(enriched_plis));

    return inverted_cluster_maps;
}

void CFDFinder::RegisterResults(std::shared_ptr<ResultStrategy> result_receiver,
                                InvertedClusterMaps inverted_cluster_maps) {
    auto results = result_receiver->TakeAllResults();
    for (auto&& result : results) {
        if (result.embedded_fd_.lhs_.count() > max_lhs_) {
            continue;
        }
        Vertical lhs_v(schema_.get(), std::move(result.embedded_fd_.lhs_));
        Column rhs_c(schema_.get(), schema_->GetColumn(result.embedded_fd_.rhs_)->GetName(),
                     result.embedded_fd_.rhs_);

        cfd_collection_.emplace_back(std::move(lhs_v), std::move(rhs_c), result.patterns_, schema_,
                                     inverted_cluster_maps);
    }
}

std::vector<Cluster> CFDFinder::EnrichPLI(model::PLI const& pli, int num_tuples) const {
    std::unordered_set<int> existing_elements;
    existing_elements.reserve(pli.GetSize());

    auto const& original_clusters = pli.GetIndex();
    for (auto const& cluster : original_clusters) {
        existing_elements.insert(cluster.begin(), cluster.end());
    }
    size_t missing_count = num_tuples - existing_elements.size();

    std::vector<Cluster> enriched_clusters;
    enriched_clusters.reserve(original_clusters.size() + missing_count);
    enriched_clusters.assign(original_clusters.begin(), original_clusters.end());

    for (int i = 0; i < num_tuples; ++i) {
        if (!existing_elements.contains(i)) {
            enriched_clusters.emplace_back(1, i);
        }
    }

    return enriched_clusters;
}

void CFDFinder::EnrichCompressedRecords(EnrichedPLIs enriched_plis) const {
    for (size_t tuple_id = 0; tuple_id < compressed_records_->size(); ++tuple_id) {
        auto& tuple = compressed_records_->at(tuple_id);
        for (size_t attr = 0; attr < tuple.size(); ++attr) {
            if (hy::PLIUtil::IsSingletonCluster(tuple[attr])) {
                auto const& clusters = enriched_plis[attr];

                for (int cluster_id = clusters.size() - 1; cluster_id >= 0; --cluster_id) {
                    if (clusters[cluster_id][0] == static_cast<int>(tuple_id)) {
                        tuple[attr] = cluster_id;
                        break;
                    }
                }
            }
        }
    }
}

CFDFinder::Lattice CFDFinder::GetLattice() {
    auto const positive_cover_tree =
            std::make_shared<hyfd::fd_tree::FDTree>(schema_->GetNumColumns());

    Sampler sampler(plis_, compressed_records_, threads_num_);
    Inductor inductor(positive_cover_tree);
    Validator validator(positive_cover_tree, plis_, compressed_records_, threads_num_);

    hy::IdPairs comparison_suggestions;

    while (true) {
        auto non_fds = sampler.GetNonFDs(comparison_suggestions);

        inductor.UpdateFdTree(std::move(non_fds));

        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }
    }

    auto fds = positive_cover_tree->FillFDs();

    std::ranges::sort(fds, std::greater<>{},
                      [](RawFD const& raw_fd) { return raw_fd.lhs_.count(); });

    std::list<Candidate> candidates;

    candidates.splice(candidates.end(), inductor.FillMaxNonFDs());
    candidates.splice(candidates.end(), validator.FillMaxNonFDs());

    for (auto const& fd : fds) {
        for (auto&& subset : utils::GenerateLhsSubsets(fd.lhs_)) {
            auto is_generalization = [&subset, rhs = fd.rhs_](auto const& candidate) {
                return rhs == candidate.rhs_ && subset.is_subset_of(candidate.lhs_);
            };

            if (!std::ranges::any_of(candidates, is_generalization)) {
                candidates.emplace_back(std::move(subset), fd.rhs_);
            }
        }
    }

    LOG_INFO("{} maximal non-FDs (initial candidates).", candidates.size());
    Lattice levels(schema_->GetNumColumns() - 1);

    for (auto&& candidate : candidates) {
        if (!rhs_filter_.empty() && !std::ranges::binary_search(rhs_filter_, candidate.rhs_)) {
            continue;
        }
        size_t level = candidate.lhs_.count() - 1;
        levels[level].insert(std::move(candidate));
    }

    return levels;
}

PatternTableau CFDFinder::GenerateTableau(boost::dynamic_bitset<> const& lhs_attributes,
                                          model::PLI const* lhs_pli, Row const& inverted_pli_rhs,
                                          std::shared_ptr<ExpansionStrategy> expansion_strategy,
                                          std::shared_ptr<PruningStrategy> pruning_strategy) {
    auto null_pattern = expansion_strategy->GenerateNullPattern(lhs_attributes);

    auto enriched_clusters = EnrichPLI(*lhs_pli, (*compressed_records_).size());
    std::list<Cluster> null_cover;

    size_t violations = 0;
    for (auto&& cluster : enriched_clusters) {
        violations += utils::CalculateViolations(cluster, inverted_pli_rhs);
        null_cover.push_back(std::move(cluster));
    }
    null_pattern.SetCover(std::move(null_cover));
    null_pattern.SetNumKeepers((*compressed_records_).size() - violations);
    null_pattern.SetSupport(null_pattern.GetNumCover());

    Frontier frontier;
    frontier.Emplace(std::move(null_pattern));

    std::vector<Pattern> tableau;

    while (!frontier.Empty() && !pruning_strategy->HasEnoughPatterns(tableau)) {
        auto current_pattern = frontier.Poll();

        if (pruning_strategy->IsPatternWorthAdding(current_pattern)) {
            pruning_strategy->AddPattern(current_pattern);

            Frontier new_frontier;

            while (!frontier.Empty()) {
                auto pattern = frontier.Poll();
                pattern.UpdateCover(current_pattern);
                pattern.UpdateKeepers(inverted_pli_rhs);
                if (pruning_strategy->IsPatternWorthConsidering(pattern)) {
                    new_frontier.Emplace(std::move(pattern));
                }
            }

            tableau.push_back(std::move(current_pattern));
            frontier.Swap(new_frontier);

        } else {
            pruning_strategy->ExpandPattern(current_pattern);
            for (auto&& child : expansion_strategy->GetChildPatterns(current_pattern)) {
                if (!pruning_strategy->ValidForProcessing(child)) {
                    continue;
                }
                pruning_strategy->ProcessChild(child);
                if (frontier.Contains(child)) {
                    continue;
                }
                child.SetCover(DetermineCover(child, current_pattern, *compressed_records_));
                child.UpdateKeepers(inverted_pli_rhs);
                child.SetSupport(child.GetNumCover());

                if (pruning_strategy->IsPatternWorthConsidering(child)) {
                    frontier.Emplace(std::move(child));
                }
            }
        }
    }

    return PatternTableau(std::move(tableau), (*compressed_records_).size());
}

std::list<Cluster> CFDFinder::DetermineCover(Pattern const& child_pattern,
                                             Pattern const& current_pattern,
                                             Rows const& records) const {
    std::list<Cluster> result;
    auto const& cover = current_pattern.GetCover();

    for (auto const& cluster : cover) {
        auto const& tuple = records[cluster[0]];
        if (child_pattern.Matches(tuple)) {
            result.push_back(cluster);
        }
    }
    return result;
}

}  // namespace algos::cfdfinder
