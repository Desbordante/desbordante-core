#include "cfdfinder.h"

#include <algorithm>
#include <chrono>
#include <unordered_set>

#include <boost/dynamic_bitset.hpp>
#include <easylogging++.h>

#include "config/equal_nulls/option.h"
#include "config/equal_nulls/type.h"
#include "config/indices/option.h"
#include "config/max_lhs/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "config/thread_number/option.h"
#include "model/entries.h"
#include "model/expansion_strategies.h"
#include "model/pruning_strategies.h"
#include "util/frontier.h"
#include "util/inductor.h"
#include "util/preprocessor.h"
#include "util/sampler.h"
#include "util/validator.h"

namespace {

using BitSet = boost::dynamic_bitset<>;

using Candidate = algos::cfdfinder::Candidate;

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs) {
    std::list<BitSet> subsets;

    for (size_t i = lhs.find_first(); i != BitSet::npos; i = lhs.find_next(i)) {
        auto subset = lhs;
        subset.flip(i);

        if (subset.any()) {
            subsets.push_back(std::move(subset));
        }
    }

    return subsets;
}

void AddLhsSubsets(Candidate const& candidate, std::unordered_set<Candidate>& level) {
    for (auto&& subset : GenerateLhsSubsets(candidate.lhs_)) {
        level.emplace(std::move(subset), candidate.rhs_);
    }
}

}  // namespace

namespace algos::cfdfinder {

CFDFinder::CFDFinder() : Algorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable, kMaximumLhs, kLimitPliCache, kEqualNulls});
}

void CFDFinder::ResetState() {
    cfd_collection_.clear();
}

void CFDFinder::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kCfdMinimumSupport, kCfdMinimumConfidence, kMaximumLhs,
                          kCfdExpansionStrategy, kCfdPruningStrategy});
};

void CFDFinder::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_expansion_strategy = [](Expansion expansion_strategy) {
        return expansion_strategy == +Expansion::constant ||
               expansion_strategy == +Expansion::range ||
               expansion_strategy == +Expansion::negative_constant;
    };

    auto legacy_eq = [](Pruning pruning_strategy) { return pruning_strategy == +Pruning::legacy; };
    auto support_independent_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == +Pruning::support_independent;
    };
    auto partial_fd_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == +Pruning::partial_fd;
    };
    auto rhs_filter_eq = [](Pruning pruning_strategy) {
        return pruning_strategy == +Pruning::rhs_filter;
    };
    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kRhsIndicesOpt(&rhs_filter_, get_schema_cols));

    RegisterOption(Option{&min_confidence_, kCfdMinimumConfidence, kDCfdMinimumConfidence, 1.0});
    RegisterOption(Option{&min_support_, kCfdMinimumSupport, kDCfdMinimumSupport, 0.8});
    RegisterOption(Option{&max_g1_, kMaximumG1, kDMaximumG1, 0.1});
    RegisterOption(Option{&max_patterns_, kPatternTreshold, kDPatternTreshold, 1000u});
    RegisterOption(Option{&min_support_gain_, kMinSupportGain, kDMinSupportGain, 1.0});
    RegisterOption(
            Option{&max_level_support_drop_, kMaxLevelSupportDrop, kDMaxLevelSupportDrop, 1.0});
    RegisterOption(Option{&limit_pli_cache_, kLimitPliCache, kDLimitPliCache, 50000u});
    RegisterOption(Option{&expansion_strategy_, kCfdExpansionStrategy, kDCfdExpansionStrategy}
                           .SetValueCheck(check_expansion_strategy));
    RegisterOption(
            Option{&pruning_strategy_, kCfdPruningStrategy, kDCfdPruningStrategy}
                    .SetConditionalOpts({{legacy_eq, {kCfdMinimumSupport, kCfdMinimumConfidence}},
                                         {support_independent_eq,
                                          {kPatternTreshold, kMinSupportGain, kMaxLevelSupportDrop,
                                           kCfdMinimumConfidence}},
                                         {partial_fd_eq, {kMaximumG1}},
                                         {rhs_filter_eq,
                                          {kPatternTreshold, kMinSupportGain, kMaxLevelSupportDrop,
                                           kCfdMinimumConfidence, kRhsIndices}}}));
}

unsigned long long CFDFinder::ExecuteInternal() {
    using algos::cfdfinder::Preprocess;
    auto start_time = std::chrono::system_clock::now();

    auto [plis, inverted_plis, compressed_records] = Preprocess(relation_.get());
    auto const inverted_plis_shared = std::make_shared<hy::Columns>(std::move(inverted_plis));
    auto const plis_shared = std::make_shared<hy::PLIs>(std::move(plis));
    auto compressed_records_shared = std::make_shared<hy::Rows>(std::move(compressed_records));

    auto levels = GetLattice(plis_shared, compressed_records_shared);

    auto const& cluster_maps = relation_->GetClusterMaps();

    EnrichedPLIs enriched_plis;
    InvertedClusterMaps inverted_cluster_maps;
    enriched_plis.reserve(relation_->GetNumColumns());
    inverted_cluster_maps.reserve(relation_->GetNumColumns());

    for (size_t i = 0; i < relation_->GetNumColumns(); i++) {
        auto enriched_pli = EnrichPLI(plis_shared->at(i), relation_->GetNumRows());
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

    // если хранить единичные кластеры в model::PositionListIndex,
    // хотя их интерфейс не предполагает этого, то можно
    // обойтись без этого метода
    EnrichCompressedRecords(compressed_records_shared, enriched_plis);

    std::unique_ptr<ExpansionStrategy> expansion_stategy =
            InitExpansionStrategy(compressed_records_shared, inverted_cluster_maps);
    std::unique_ptr<PruningStrategy> pruning_stategy = InitPruningStrategy(inverted_plis_shared);
    PLICache pli_cache(limit_pli_cache_, relation_->GetSchema());

    size_t num_results = 0;
    int height = levels.size() - 1;
    while (height >= 0) {
        auto start_level_time = std::chrono::system_clock::now();
        for (auto&& candidate : levels[height]) {
            auto const lhs_pli = GetLhsPli(pli_cache, candidate.lhs_, plis_shared);
            auto inverted_pli_rhs = inverted_plis_shared->at(candidate.rhs_);
            pruning_stategy->StartNewTableau(candidate);
            PatternTableau pattern_tableau = GenerateTableau(
                    candidate.lhs_, lhs_pli.get(), inverted_pli_rhs, compressed_records_shared,
                    expansion_stategy.get(), pruning_stategy.get());

            if (!pruning_stategy->ContinueGeneration(pattern_tableau)) {
                continue;
            }
            if (height > 0) {
                AddLhsSubsets(candidate, levels[height - 1]);
            }
            num_results++;
            RegisterCFD(std::move(candidate), pattern_tableau, inverted_cluster_maps);
        }
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - start_level_time);
        LOG(INFO) << "Finished level " << height << "(" << elapsed_seconds.count() << ", "
                  << num_results << " results)";
        height--;
    }
    LOG(INFO) << "Total PLI computations: "
              << pli_cache.GetTotalMisses() + pli_cache.GetFullHits() + pli_cache.GetPartialHits();
    LOG(INFO) << "Total PLI cache misses: " << pli_cache.GetTotalMisses();
    LOG(INFO) << "Total PLI cache hits: " << pli_cache.GetFullHits() + pli_cache.GetPartialHits();
    LOG(INFO) << "Total full PLI cache hits: " << pli_cache.GetFullHits();
    LOG(INFO) << "Total partial PLI cache hits: " << pli_cache.GetPartialHits();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    unsigned long long apriori_millis = elapsed_milliseconds.count();
    return apriori_millis;
}

std::vector<Cluster> CFDFinder::EnrichPLI(model::PositionListIndex const* pli, int num_tuples) {
    auto const& original_clusters = pli->GetIndex();
    std::vector<Cluster> enriched_clusters;

    std::unordered_set<int> existing_elements;

    size_t total_elements = 0;
    for (auto const& cluster : original_clusters) {
        total_elements += cluster.size();
    }
    existing_elements.reserve(total_elements);

    for (auto const& cluster : original_clusters) {
        existing_elements.insert(cluster.begin(), cluster.end());
    }

    size_t missing_count = num_tuples - existing_elements.size();
    enriched_clusters.reserve(original_clusters.size() + missing_count);
    enriched_clusters.assign(original_clusters.begin(), original_clusters.end());

    for (int i = 0; i < num_tuples; ++i) {
        if (!existing_elements.contains(i)) {
            enriched_clusters.emplace_back(1, i);
        }
    }

    return enriched_clusters;
}

void CFDFinder::EnrichCompressedRecords(hy::RowsPtr& compressed_records,
                                        EnrichedPLIs const& enriched_plis) {
    for (size_t tuple_id = 0; tuple_id < compressed_records->size(); ++tuple_id) {
        auto& tuple = compressed_records->at(tuple_id);
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

std::shared_ptr<model::PositionListIndex const> CFDFinder::GetLhsPli(
        PLICache& pli_cache, boost::dynamic_bitset<> const& lhs, hy::PLIsPtr const& plis) {
    if (auto cached = pli_cache.Get(lhs)) {
        pli_cache.AddFullHit();
        return cached;
    }

    boost::dynamic_bitset<> current_lhs(lhs.size());
    std::shared_ptr<model::PositionListIndex const> result;

    for (size_t index = lhs.find_first(); index != boost::dynamic_bitset<>::npos;
         index = lhs.find_next(index)) {
        current_lhs.flip(index);

        if (auto cached = pli_cache.Get(current_lhs)) {
            pli_cache.AddPartialHit();
            result = std::move(cached);
            continue;
        }

        auto& pli = plis->at(index);
        if (!result) {
            result = std::shared_ptr<model::PositionListIndex const>(
                    pli, [](model::PositionListIndex*) {});
        } else {
            pli_cache.AddMiss();

            auto intersected = result->Intersect(pli);
            auto shared_intersected =
                    std::shared_ptr<model::PositionListIndex>(std::move(intersected));

            pli_cache.Put(current_lhs, shared_intersected);

            result = pli_cache.Get(current_lhs) ? pli_cache.Get(current_lhs) : shared_intersected;
        }
    }

    return result;
}

void CFDFinder::LoadDataInternal() {
    relation_ = CFDFinderRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: CFD mining is meaningless.");
    }
}

CFDFinder::Lattice CFDFinder::GetLattice(hy::PLIsPtr const& plis,
                                         hy::RowsPtr const& compressed_records) {
    auto const positive_cover_tree =
            std::make_shared<hyfd::fd_tree::FDTree>(relation_->GetNumColumns());

    Sampler sampler(plis, compressed_records, threads_num_);
    Inductor inductor(positive_cover_tree);
    Validator validator(positive_cover_tree, plis, compressed_records, threads_num_);

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

    std::sort(fds.begin(), fds.end(), [](RawFD const& first, RawFD const& second) {
        return first.lhs_.count() > second.lhs_.count();
    });

    std::list<Candidate> candidates;

    candidates.splice(candidates.end(), inductor.FillMaxNonFDs());
    candidates.splice(candidates.end(), validator.FillMaxNonFDs());

    for (auto&& [lhs, rhs] : fds) {
        for (auto&& subset : GenerateLhsSubsets(std::move(lhs))) {
            if (!std::any_of(candidates.begin(), candidates.end(), [&](auto const& candidate) {
                    return rhs == candidate.rhs_ && subset.is_subset_of(candidate.lhs_);
                })) {
                candidates.emplace_back(std::move(subset), rhs);
            }
        }
    }

    LOG(INFO) << candidates.size() << " maximal non-FDs (initial candidates).";
    Lattice levels(relation_->GetNumColumns() - 1);

    for (auto&& candidate : candidates) {
        size_t level = candidate.lhs_.count() - 1;
        levels[level].insert(std::move(candidate));
    }

    return levels;
}

void CFDFinder::RegisterCFD(Candidate candidate, PatternTableau const& tableau,
                            InvertedClusterMaps const& inverted_cluster_maps) {
    if (candidate.lhs_.count() > max_lhs_) {
        return;
    }

    auto const* const schema = relation_->GetSchema();
    Vertical lhs_v(schema, std::move(candidate.lhs_));
    Column rhs_c(schema, schema->GetColumn(candidate.rhs_)->GetName(), candidate.rhs_);

    std::vector<std::vector<std::string>> patterns_values;

    for (auto const& pattern : tableau.GetPatterns()) {
        std::vector<std::string> entries = GetEntriesString(pattern, inverted_cluster_maps);
        patterns_values.push_back(std::move(entries));
    }

    cfd_collection_.emplace_back(std::move(lhs_v), std::move(rhs_c), tableau,
                                 relation_->GetSharedPtrSchema(), std::move(patterns_values));
}

std::vector<std::string> CFDFinder::GetEntriesString(
        Pattern const& pattern, InvertedClusterMaps const& inverted_cluster_maps) const {
    std::vector<std::string> result;
    static std::string const kNullRepresentation = "null";
    static std::string const kNegationSign = "¬";

    for (auto const& [id, entry] : pattern.GetEntries()) {
        auto const& inverted_cluster_map = inverted_cluster_maps[id];
        switch (entry->GetType()) {
            case EntryType::kVariable:
                result.push_back("_");
                break;
            case EntryType::kConstant: {
                auto const* constant_entry = static_cast<ConstantEntry const*>(entry.get());
                std::string value =
                        inverted_cluster_map.find(constant_entry->GetConstant())->second;
                if (value.empty()) {
                    value = kNullRepresentation;
                }

                result.push_back(std::move(value));
                break;
            }
            case EntryType::kNegativeConstant: {
                auto const* neg_constant_entry =
                        static_cast<NegativeConstantEntry const*>(entry.get());
                std::string value =
                        inverted_cluster_map.find(neg_constant_entry->GetConstant())->second;

                value = (!value.empty()) ? kNegationSign + value
                                         : kNegationSign + kNullRepresentation;
                result.push_back(std::move(value));
                break;
            }
            case EntryType::kRange: {
                auto const* range_entry = static_cast<RangeEntry const*>(entry.get());

                std::string lower_bound;
                std::string upper_bound;

                lower_bound = inverted_cluster_map.find(range_entry->GetLowerBound())->second;
                if (lower_bound.empty()) {
                    lower_bound = kNullRepresentation;
                }
                upper_bound = inverted_cluster_map.find(range_entry->GetUpperBound())->second;
                if (upper_bound.empty()) {
                    upper_bound = kNullRepresentation;
                }

                result.push_back("[" + lower_bound + " - " + upper_bound + "]");
                break;
            }
        }
    }

    return result;
}

PatternTableau CFDFinder::GenerateTableau(boost::dynamic_bitset<> const& lhs_attributes,
                                          model::PositionListIndex const* lhs_pli,
                                          hy::Row const& inverted_pli_rhs,
                                          hy::RowsPtr const& compressed_records_shared,
                                          ExpansionStrategy* const expansion_strategy,
                                          PruningStrategy* const pruning_strategy) {
    if (PatternDebugController::IsDebugEnabled()) {
        PatternDebugController::ResetCounter();
    }
    auto null_pattern = expansion_strategy->GenerateNullPattern(lhs_attributes);
    size_t violations = 0;

    auto enriched_clusters = EnrichPLI(lhs_pli, relation_->GetNumRows());
    std::list<Cluster> null_cover;

    for (auto&& cluster : enriched_clusters) {
        violations += util::CalculateViolations(cluster, inverted_pli_rhs);
        null_cover.push_back(std::move(cluster));
    }
    null_pattern.SetCover(std::move(null_cover));
    null_pattern.SetNumKeepers(relation_->GetNumRows() - violations);
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
                auto pattern(frontier.Poll());
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
                child.SetCover(DetermineCover(child, current_pattern, compressed_records_shared));
                child.UpdateKeepers(inverted_pli_rhs);
                child.SetSupport(child.GetNumCover());

                if (pruning_strategy->IsPatternWorthConsidering(child)) {
                    frontier.Emplace(std::move(child));
                }
            }
        }
    }

    return PatternTableau(std::move(tableau), relation_->GetNumRows());
}

std::list<Cluster> CFDFinder::DetermineCover(Pattern const& child_pattern,
                                             Pattern const& current_pattern,
                                             hy::RowsPtr const& compressed_records) const {
    std::list<Cluster> result;
    auto const& cover = current_pattern.GetCover();
    for (auto const& cluster : cover) {
        auto const& tuple = compressed_records->at(cluster[0]);
        if (child_pattern.Matches(tuple)) {
            result.push_back(cluster);
        }
    }

    return result;
}

std::unique_ptr<ExpansionStrategy> CFDFinder::InitExpansionStrategy(
        hy::RowsPtr const& pli_records, InvertedClusterMaps const& inverted_cluster_maps) {
    switch (expansion_strategy_) {
        case Expansion::constant:
            return std::make_unique<ConstantExpansion>(pli_records);
        case Expansion::range:
            return std::make_unique<RangePatternExpansion>(inverted_cluster_maps);
        case Expansion::negative_constant:
            return std::make_unique<PositiveNegativeConstantExpansion>(pli_records);
        default:
            return std::make_unique<ConstantExpansion>(pli_records);
    }
}

std::unique_ptr<PruningStrategy> CFDFinder::InitPruningStrategy(hy::RowsPtr const& inverted_plis) {
    switch (pruning_strategy_) {
        case Pruning::legacy:
            return std::make_unique<LegacyPruning>(min_support_, min_confidence_,
                                                   relation_->GetNumRows());
        case Pruning::support_independent:
            return std::make_unique<SupportIndependentPruning>(
                    max_patterns_, min_support_gain_, max_level_support_drop_, min_confidence_,
                    relation_->GetNumColumns());
        case Pruning::partial_fd:
            return std::make_unique<PartialFdPruning>(relation_->GetNumRows(), max_g1_,
                                                      inverted_plis);
        case Pruning::rhs_filter:
            return std::make_unique<RhsFilterPruning>(max_patterns_, min_support_gain_,
                                                      max_level_support_drop_, min_confidence_,
                                                      relation_->GetNumColumns(), rhs_filter_);
        default:
            return std::make_unique<LegacyPruning>(min_support_, min_confidence_,
                                                   relation_->GetNumRows());
    }
}
}  // namespace algos::cfdfinder