#include "algorithms/fd/fd_verifier/dynamic_stats_calculator.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

#include <easylogging++.h>

namespace algos::fd_verifier {

void DynamicStatsCalculator::CalculateStatistics(model::DynPLI const* lhs_pli,
                                                 model::DynPLI const* rhs_pli) {
    model::DynPLI::ClusterCollection const& lhs_clusters = lhs_pli->GetClusters();
    std::shared_ptr<std::vector<int> const> pt_shared = rhs_pli->GetCachedProbingTable();
    std::vector<int> const& rhs_pt = *pt_shared;
    size_t num_tuples_conflicting_on_rhs = 0;

    for (auto const& [_, cluster] : lhs_clusters) {
        if (cluster.size() == 1) continue;
        Frequencies frequencies = model::DynPLI::CreateFrequencies(cluster, rhs_pt);
        size_t num_distinct_rhs_values = CalculateNumDistinctRhsValues(frequencies, cluster.size());
        if (num_distinct_rhs_values == 1) continue;
        num_tuples_conflicting_on_rhs +=
                CalculateNumTuplesConflictingOnRhsInCluster(frequencies, cluster.size());
        num_error_rows_ += cluster.size();
        highlights_.emplace_back(cluster, num_distinct_rhs_values,
                                 CalculateNumMostFrequentRhsValue(frequencies));
    }

    assert(!highlights_.empty());
    size_t num_rows = table_data_->GetNumRowsActual();
    error_ = (double)num_tuples_conflicting_on_rhs / (num_rows * num_rows - num_rows);
}

size_t DynamicStatsCalculator::CalculateNumMostFrequentRhsValue(Frequencies const& frequencies) {
    // Only singletons in cluster
    if (frequencies.empty()) {
        return 1;
    }
    auto comp = [](auto const& a, auto const& b) { return a.second < b.second; };
    return std::max_element(frequencies.begin(), frequencies.end(), comp)->second;
}

size_t DynamicStatsCalculator::CalculateNumTuplesConflictingOnRhsInCluster(
        Frequencies const& frequencies, size_t cluster_size) {
    size_t num_tuples_conflicting_on_rhs = cluster_size * (cluster_size - 1);
    for (auto const& [_, number_of_values] : frequencies) {
        if (number_of_values > 1) {
            num_tuples_conflicting_on_rhs -= number_of_values * (number_of_values - 1);
        }
    }
    return num_tuples_conflicting_on_rhs;
}

size_t DynamicStatsCalculator::CalculateNumDistinctRhsValues(Frequencies const& frequencies,
                                                             size_t cluster_size) {
    size_t num_non_singleton_rhs_values = std::accumulate(
            frequencies.begin(), frequencies.end(), 0U,
            [](unsigned total, auto const& freq_iter) { return total + freq_iter.second; });
    return frequencies.size() + cluster_size - num_non_singleton_rhs_values;
}

void DynamicStatsCalculator::SortHighlights(HighlightCompareFunction const& compare) {
    std::sort(highlights_.begin(), highlights_.end(), compare);
}

auto DynamicStatsCalculator::CompareHighlightsByProportionAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetMostFrequentRhsValueProportion() < h2.GetMostFrequentRhsValueProportion();
    };
}

auto DynamicStatsCalculator::CompareHighlightsByProportionDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetMostFrequentRhsValueProportion() > h2.GetMostFrequentRhsValueProportion();
    };
}

auto DynamicStatsCalculator::CompareHighlightsByNumAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetNumDistinctRhsValues() < h2.GetNumDistinctRhsValues();
    };
}

auto DynamicStatsCalculator::CompareHighlightsByNumDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetNumDistinctRhsValues() > h2.GetNumDistinctRhsValues();
    };
}

auto DynamicStatsCalculator::CompareHighlightsBySizeAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetCluster().size() < h2.GetCluster().size();
    };
}

auto DynamicStatsCalculator::CompareHighlightsBySizeDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetCluster().size() > h2.GetCluster().size();
    };
}

}  // namespace algos::fd_verifier
