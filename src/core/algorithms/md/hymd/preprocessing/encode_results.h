#pragma once

#include <cstddef>
#include <set>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/preprocessing/valid_table_results.h"
#include "algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd::preprocessing {

namespace detail {
template <typename ResultType>
EnumeratedValidTableResults EnumerateResults(
        ValidTableResults<ResultType> const& results,
        std::unordered_map<ResultType, ColumnClassifierValueId> const& id_map) {
    EnumeratedValidTableResults enumerated;
    enumerated.reserve(results.size());
    for (auto const& [row_results, valid_records_number] : results) {
        ValidRowResults<ColumnClassifierValueId> row;
        for (auto const& [result, value_id] : row_results) {
            ColumnClassifierValueId const ccv_id = id_map.find(result)->second;
            if (ccv_id != kLowestCCValueId) row.emplace_back(ccv_id, value_id);
        }
        enumerated.emplace_back(std::move(row), valid_records_number);
    }
    return enumerated;
}

template <typename Order, typename ResultType>
std::pair<std::vector<ResultType>, std::unordered_map<ResultType, ColumnClassifierValueId>>
CreateCCValueIdMap(std::span<ResultType const> additional_results,
                   ValidTableResults<ResultType> const& valid_table_results) {
    std::set<ResultType, Order> results_set;
    results_set.insert(additional_results.begin(), additional_results.end());
    for (auto const& [res_vec, _] : valid_table_results) {
        for (auto const& [result, _] : res_vec) {
            results_set.insert(result);
        }
    }
    std::vector<ResultType> ordered_results;
    std::unordered_map<ResultType, ColumnClassifierValueId> value_map(results_set.size());
    ordered_results.reserve(results_set.size());
    ordered_results.insert(ordered_results.end(), results_set.begin(), results_set.end());

    ColumnClassifierValueId ccv_id = 0;
    for (ResultType const& result : ordered_results) {
        value_map[result] = ccv_id++;
    }
    return {std::move(ordered_results), std::move(value_map)};
}
}  // namespace detail

template <typename ResultType, typename Order = std::less<ResultType>>
std::pair<std::vector<ResultType>, EnumeratedValidTableResults> EncodeResults(
        ValidTableResults<ResultType> valid_table_results,
        std::span<ResultType const> additional_results) {
    auto [classifier_values, ccv_id_map] =
            detail::CreateCCValueIdMap<Order>(additional_results, valid_table_results);
    auto enumerated = detail::EnumerateResults<ResultType>(valid_table_results, ccv_id_map);
    return {std::move(classifier_values), std::move(enumerated)};
}

}  // namespace algos::hymd::preprocessing
