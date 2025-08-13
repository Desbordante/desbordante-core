#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <ranges>
#include <set>
#include <unordered_map>

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/meaningful_table_results.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace detail {
template <typename ResultType>
EnumeratedMeaningfulDataResults EnumerateMeaningfulResults(
        MeaningfulDataResults<ResultType> const& results,
        std::unordered_map<ResultType, RecordClassifierValueId> const& rcv_id_map,
        bool least_element_found) {
    EnumeratedMeaningfulDataResults enumerated;
    enumerated.reserve(results.size());
    for (auto const& row_results : results) {
        using EnumeratedRowType = MeaningfulLeftValueResults<RecordClassifierValueId>;
        EnumeratedRowType& enumerated_row = enumerated.emplace_back();
        // TODO: should store if least element was found for each row instead.
        if (least_element_found) enumerated_row.reserve(row_results.size());
        for (auto const& [comparison_result, rt_pvalue_id, cluster_size] : row_results) {
            auto it = rcv_id_map.find(comparison_result);
            // (Non-universal) Total decision boundary found, don't store.
            if (it == rcv_id_map.end()) {
                assert(!least_element_found);
                continue;
            }

            RecordClassifierValueId const rcv_id = it->second;
            assert(rcv_id != kLowestRCValueId);
            enumerated_row.emplace_back(rcv_id, rt_pvalue_id, cluster_size);
        }
    }
    return enumerated;
}

template <typename ComparisonResultType>
std::pair<std::vector<ComparisonResultType>,
          std::unordered_map<ComparisonResultType, RecordClassifierValueId>>
GenerateResultRCVIDs(MeaningfulDataResults<ComparisonResultType> const& meaningful_table_results,
                     orders::TotalOrder<ComparisonResultType> const* order_ptr,
                     bool least_element_found) {
    std::set<ComparisonResultType, orders::OrderCompareWrapper<ComparisonResultType>>
            comparison_results{orders::OrderCompareWrapper<ComparisonResultType>(order_ptr)};

    // Results with the least element are not supposed to be stored, store it here.
    if (least_element_found) comparison_results.insert(order_ptr->LeastElement());
    for (auto const& res_vec : meaningful_table_results) {
        for (auto const& [result, rt_pvid, cluster_size] : res_vec) {
            comparison_results.insert(result);
        }
    }
    // TODO: If the greatest element was not found, the algorithm can optimize the non-zero min
    // support case (LHS with the greatest classifier value can be ignored or validation can be
    // skipped).
    comparison_results.emplace(order_ptr->GreatestElement());

    std::vector<ComparisonResultType> ordered_comparison_results(comparison_results.begin(),
                                                                 comparison_results.end());
    std::unordered_map<ComparisonResultType, RecordClassifierValueId>
            comparison_result_to_rcv_id_map(comparison_results.size() - 1);

    RecordClassifierValueId current_rcv_id = 1;
    // Another approach is to store this first useless result (i.e. total decision boundary) and
    // tell the compiler in the function above that we will always find the key in the map, but at
    // least GCC is not smart enough to optimize out the dead code in std::unordered_map in this
    // case, so opting for a safer approach here.
    for (ComparisonResultType const& comparison_result :
         std::ranges::drop_view(ordered_comparison_results, 1)) {
        assert(current_rcv_id != std::numeric_limits<RecordClassifierValueId>::max());
        comparison_result_to_rcv_id_map[comparison_result] = current_rcv_id++;
    }
    return {std::move(ordered_comparison_results), std::move(comparison_result_to_rcv_id_map)};
}
}  // namespace detail

template <typename ResultType>
std::tuple<std::vector<ResultType>, EnumeratedMeaningfulDataResults, bool> EncodeResults(
        MeaningfulDataResults<ResultType> const& meaningful_table_results,
        orders::TotalOrder<ResultType> const* order_ptr, bool least_element_found) {
    auto [record_match_comparison_results, rcv_id_map] =
            detail::GenerateResultRCVIDs(meaningful_table_results, order_ptr, least_element_found);
    EnumeratedMeaningfulDataResults enumerated = detail::EnumerateMeaningfulResults(
            meaningful_table_results, rcv_id_map, least_element_found);
    return {std::move(record_match_comparison_results), std::move(enumerated), least_element_found};
}
}  // namespace algos::hymde::record_match_indexes::calculators
