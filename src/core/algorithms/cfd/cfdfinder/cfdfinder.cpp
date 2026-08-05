#include "core/algorithms/cfd/cfdfinder/cfdfinder.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <future>
#include <ranges>
#include <tuple>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/model/expansion_strategies.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/inductor.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/preprocessor.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/sampler.h"
#include "core/algorithms/cfd/cfdfinder/model/hyfd/validator.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/constant_entry.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/negative_constant_entry.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning_strategies.h"
#include "core/algorithms/cfd/cfdfinder/model/result_strategies.h"
#include "core/algorithms/cfd/cfdfinder/types/frontier.h"
#include "core/algorithms/cfd/cfdfinder/util/lhs_utils.h"
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

void CFDFinder::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kMaximumLhs, kLimitPliCache, kCfdExpansionStrategy, kCfdPruningStrategy,
                          kCfdResultStrategy, kThreads, kRhsIndices});
}

void CFDFinder::LoadDataInternal() {
    schema_ = RelationalSchema::CreateFrom(*input_table_);
    relation_ = CFDFinderRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: CFD mining is meaningless.");
    }
}

void CFDFinder::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));

    auto get_schema_cols = [this]() { return schema_->GetNumColumns(); };
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

    auto legacy_eq = [](Pruning pruning_strategy) { return pruning_strategy == Pruning::kLegacy; };
    auto support_independent_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == Pruning::kSupportIndependent;
    };
    auto partial_fd_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == Pruning::kPartialFd;
    };
    RegisterOption(
            Option{&pruning_strategy_, kCfdPruningStrategy, kDCfdPruningStrategy}
                    .SetConditionalOpts({{legacy_eq, {kCfdMinimumSupport, kCfdMinimumConfidence}},
                                         {support_independent_eq,
                                          {kMaxPatterns, kMinSupportGain, kMaxLevelSupportDrop,
                                           kCfdMinimumConfidence}},
                                         {partial_fd_eq, {kMaximumG1}}}));
};

void CFDFinder::ExecuteInternal() {
    std::tie(plis_, inverted_plis_, compressed_records_) = Preprocess(relation_.get());

    auto levels = GetLattice();
    auto inverted_cluster_maps = BuildEnrichedStructures();

    PLICache pli_cache(limit_pli_cache_, schema_.get(), plis_);

    if (threads_num_ > 1) {
        TraverseLatticePar(std::move(inverted_cluster_maps), std::move(levels), pli_cache);
    } else {
        TraverseLatticeSeq(std::move(inverted_cluster_maps), std::move(levels), pli_cache);
    }

    LOG_INFO("Total PLI computations: {}",
             pli_cache.GetTotalMisses() + pli_cache.GetFullHits() + pli_cache.GetPartialHits());
    LOG_INFO("Total PLI cache misses: {}", pli_cache.GetTotalMisses());
    LOG_INFO("Total PLI cache hits: {}", pli_cache.GetFullHits() + pli_cache.GetPartialHits());
    LOG_INFO("Total full PLI cache hits: {}", pli_cache.GetFullHits());
    LOG_INFO("Total partial PLI cache hits: {}", pli_cache.GetPartialHits());
}

void CFDFinder::TraverseLatticeSeq(InvertedClusterMaps inverted_cluster_maps, Lattice&& levels,
                                   PLICache& pli_cache) {
    auto pruning_strategy = InitPruningStrategy(inverted_plis_);
    auto expansion_strategy = InitExpansionStrategy(compressed_records_, inverted_cluster_maps);
    auto result_receiver = InitResultStrategy();

    size_t num_results = 0;
    int height = levels.size();
    --height;

    auto traverse_level_seq = [&]() {
        for (auto&& candidate : levels[height]) {
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
            result_receiver->ReceiveResult(candidate, std::move(pattern_tableau));
        }
    };

    while (height >= 0) {
        auto start_level = std::chrono::system_clock::now();

        traverse_level_seq();

        auto process_level_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - start_level);
        LOG_INFO("Finished level {} ({}s), {} results.", height, process_level_seconds.count(),
                 num_results);
        --height;
    }

    RegisterResults(result_receiver->TakeAllResults(), std::move(inverted_cluster_maps));
}

void CFDFinder::TraverseLatticePar(InvertedClusterMaps inverted_cluster_maps, Lattice&& levels,
                                   PLICache& pli_cache) {
    auto pruning_strategy = InitPruningStrategy(inverted_plis_);
    auto expansion_strategy = InitExpansionStrategy(compressed_records_, inverted_cluster_maps);
    auto result_receiver = InitResultStrategy();

    size_t num_results = 0;
    int height = levels.size();
    --height;

    auto process_candidate = [this, &expansion_strategy](
                                     Candidate&& candidate,
                                     std::shared_ptr<model::PLI const> const lhs_pli,
                                     std::shared_ptr<PruningStrategy> pruning_strategy)
            -> std::pair<Candidate, std::optional<PatternTableau>> {
        auto const inverted_pli_rhs = inverted_plis_->at(candidate.rhs_);
        pruning_strategy->StartNewTableau(candidate);
        PatternTableau pattern_tableau =
                GenerateTableau(candidate.lhs_, lhs_pli.get(), inverted_pli_rhs, expansion_strategy,
                                pruning_strategy);

        if (!pruning_strategy->ContinueGeneration(pattern_tableau)) {
            return {std::move(candidate), std::nullopt};
        }
        return {std::move(candidate), std::move(pattern_tableau)};
    };

    auto traverse_level_par = [&]() {
        boost::asio::thread_pool thread_pool(threads_num_);
        size_t const batch_size = static_cast<size_t>(threads_num_) * 4;
        auto& current_level = levels[height];
        while (!current_level.empty()) {
            std::vector<std::future<std::pair<Candidate, std::optional<PatternTableau>>>> futures;
            futures.reserve(std::min(batch_size, current_level.size()));

            for (size_t i = 0; i < batch_size && !current_level.empty(); ++i) {
                auto candidate = current_level.extract(current_level.begin()).value();
                auto lhs_pli = pli_cache.GetLhsPli(candidate.lhs_);

                std::packaged_task<std::pair<Candidate, std::optional<PatternTableau>>()> task(
                        [&, candidate = std::move(candidate), lhs_pli = std::move(lhs_pli),
                         pruning_strategy = pruning_strategy->Clone()]() mutable {
                            return process_candidate(std::move(candidate), lhs_pli,
                                                     std::move(pruning_strategy));
                        });
                futures.push_back(task.get_future());
                boost::asio::post(thread_pool, std::move(task));
            }
            for (auto& future : futures) {
                auto [candidate, pattern_tableau_opt] = future.get();
                if (!pattern_tableau_opt) {
                    continue;
                }
                if (height > 0) {
                    auto& target_level = levels[height - 1];
                    for (auto&& subset : utils::GenerateLhsSubsets(candidate.lhs_)) {
                        target_level.emplace(std::move(subset), candidate.rhs_);
                    }
                }
                ++num_results;
                result_receiver->ReceiveResult(std::move(candidate), *pattern_tableau_opt);
            }
        }

        thread_pool.join();
    };

    while (height >= 0) {
        auto start_level = std::chrono::system_clock::now();

        traverse_level_par();

        auto process_level_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - start_level);
        LOG_INFO("Finished level {} ({}s), {} results.", height, process_level_seconds.count(),
                 num_results);
        --height;
    }

    RegisterResults(result_receiver->TakeAllResults(), std::move(inverted_cluster_maps));
}

InvertedClusterMaps CFDFinder::BuildEnrichedStructures() const {
    size_t const num_rows = relation_->GetNumRows();
    auto const& cluster_maps = relation_->GetClusterMaps();

    EnrichedPLIs enriched_plis;
    enriched_plis.reserve(plis_->size());
    std::transform(plis_->begin(), plis_->end(), std::back_inserter(enriched_plis),
                   [&](auto const& pli) { return utils::EnrichPLI(&pli, num_rows); });

    size_t col = 0;
    auto enrich_cluster_map = [&](std::vector<Cluster> const& enriched_pli) {
        std::unordered_map<int, std::string_view> element_to_key;
        for (auto const& [key, cluster] : cluster_maps[col++]) {
            for (int elem : cluster) {
                element_to_key[elem] = key;
            }
        }

        InvertedClusterMap inverted_cluster_map;
        for (size_t cluster_id = 0; cluster_id < enriched_pli.size(); ++cluster_id) {
            int first_element = enriched_pli[cluster_id].front();

            if (auto it = element_to_key.find(first_element); it != element_to_key.end()) {
                inverted_cluster_map[cluster_id] = it->second;
            }
        }
        return inverted_cluster_map;
    };

    InvertedClusterMaps inverted_cluster_maps;
    inverted_cluster_maps.reserve(enriched_plis.size());
    std::transform(enriched_plis.begin(), enriched_plis.end(),
                   std::back_inserter(inverted_cluster_maps), std::move(enrich_cluster_map));

    EnrichCompressedRecords(std::move(enriched_plis));

    return inverted_cluster_maps;
}

void CFDFinder::RegisterResults(std::list<RawCFD> results,
                                InvertedClusterMaps inverted_cluster_maps) {
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

std::list<Candidate> CFDFinder::RunHyFdPhase() const {
    auto const positive_cover_tree =
            std::make_shared<hyfd::fd_tree::FDTree>(relation_->GetNumColumns());

    Sampler sampler(plis_, compressed_records_);
    Inductor inductor(positive_cover_tree);
    Validator validator(positive_cover_tree, plis_, compressed_records_);

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

    std::list<Candidate> max_non_fds;

    max_non_fds.splice(max_non_fds.end(), inductor.FillMaxNonFDs());

    for (auto const& fd : fds) {
        for (auto&& subset : utils::GenerateLhsSubsets(fd.lhs_)) {
            auto is_generalization = [&subset, rhs = fd.rhs_](auto const& candidate) {
                return rhs == candidate.rhs_ && subset.is_subset_of(candidate.lhs_);
            };
            if (!std::ranges::any_of(max_non_fds, is_generalization)) {
                max_non_fds.emplace_back(std::move(subset), fd.rhs_);
            }
        }
    }

    LOG_INFO("{} maximal non-FDs (initial candidates).", max_non_fds.size());

    return max_non_fds;
}

CFDFinder::Lattice CFDFinder::GetLattice() {
    auto max_non_fds = RunHyFdPhase();

    Lattice levels(relation_->GetNumColumns() - 1);

    for (auto&& candidate : max_non_fds) {
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
    Frontier frontier;
    frontier.Emplace(
            expansion_strategy->GenerateNullPattern(lhs_attributes, lhs_pli, inverted_pli_rhs));

    std::vector<Pattern> tableau;

    while (!frontier.Empty() && !pruning_strategy->HasEnoughPatterns(tableau)) {
        auto current_pattern = frontier.Poll();

        if (!pruning_strategy->TryAdding(current_pattern)) {
            expansion_strategy->Expand(std::move(current_pattern), frontier, inverted_pli_rhs,
                                       pruning_strategy);
            continue;
        }

        boost::dynamic_bitset<> used_rows_mask(relation_->GetNumRows());
        for (auto row_id : current_pattern.GetCover() | std::views::join) {
            used_rows_mask.set(row_id);
        }
        tableau.push_back(std::move(current_pattern));

        frontier.Rebuild(used_rows_mask, inverted_pli_rhs, *pruning_strategy);
    }
    return PatternTableau(std::move(tableau), relation_->GetNumRows());
}

std::shared_ptr<ExpansionStrategy> CFDFinder::InitExpansionStrategy(
        RowsPtr pli_records, InvertedClusterMaps const& inverted_cluster_maps) {
    switch (expansion_strategy_) {
        case Expansion::kConstant:
            return std::make_shared<ConstantExpansion>(std::move(pli_records));
        case Expansion::kRange:
            return std::make_shared<RangePatternExpansion>(inverted_cluster_maps,
                                                           std::move(pli_records));
        case Expansion::kNegativeConstant:
            return std::make_shared<PositiveNegativeConstantExpansion>(std::move(pli_records));
        default:
            return std::make_shared<ConstantExpansion>(std::move(pli_records));
    }
}

std::shared_ptr<PruningStrategy> CFDFinder::InitPruningStrategy(ColumnsPtr inverted_plis) {
    switch (pruning_strategy_) {
        case Pruning::kLegacy:
            return std::make_shared<LegacyPruning>(min_support_, min_confidence_,
                                                   relation_->GetNumRows());
        case Pruning::kSupportIndependent:
            return std::make_shared<SupportIndependentPruning>(max_patterns_, min_support_gain_,
                                                               max_level_support_drop_,
                                                               min_confidence_, threads_num_);
        case Pruning::kPartialFd:
            return std::make_shared<PartialFdPruning>(relation_->GetNumRows(), max_g1_,
                                                      std::move(inverted_plis));
        default:
            return std::make_shared<LegacyPruning>(min_support_, min_confidence_,
                                                   relation_->GetNumRows());
    }
}

std::shared_ptr<ResultStrategy> CFDFinder::InitResultStrategy() {
    switch (result_strategy_) {
        case Result::kDirect:
            return std::make_shared<DirectOutputStrategy>();
        case Result::kLattice:
            return std::make_shared<ResultLatticeStrategy>();
        case Result::kTree:
            return std::make_shared<ResultTreeStrategy>();
        default:
            return std::make_shared<DirectOutputStrategy>();
    }
}
}  // namespace algos::cfdfinder
