#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <utility>

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/meaningful_table_results.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/to_decision_boundaries.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/record_match_indexes/upper_set_index.h"
#include "algorithms/mde/hymde/record_match_indexes/value_matrix.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "util/desbordante_assume.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymde::record_match_indexes::calculators {
inline ValueMatrix CreateValueMatrix(EnumeratedMeaningfulDataResults const& meaningful_results) {
    ValueMatrix value_matrix;
    std::size_t const left_value_number = meaningful_results.size();
    value_matrix.reserve(left_value_number);
    for (auto const& left_pvalue_results : meaningful_results) {
        ValueMatrixRow& left_value_row = value_matrix.emplace_back(left_pvalue_results.size());
        for (auto const& [rcv_id, right_partition_value_id, cluster_size] : left_pvalue_results) {
            // Should have been excluded during enumeration.
            assert(rcv_id != kLowestRCValueId);
            left_value_row.try_emplace(right_partition_value_id, rcv_id);
        }
    }
    return value_matrix;
}

inline void SortForAllLeftValues(EnumeratedMeaningfulDataResults& meaningful_results) {
    auto result_data_comparer = [](MeaningfulResult<RecordClassifierValueId> const& p1,
                                   MeaningfulResult<RecordClassifierValueId> const& p2) {
        auto const& [rcv_id1, pvalue_id1, cluster_size1] = p1;
        auto const& [rcv_id2, pvalue_id2, cluster_size2] = p2;
        return rcv_id1 > rcv_id2 || (rcv_id1 == rcv_id2 && pvalue_id1 < pvalue_id2);
    };
    auto sort_for_left_value =
            [&result_data_comparer](
                    MeaningfulLeftValueResults<RecordClassifierValueId>& left_value_results) {
                std::ranges::sort(left_value_results, result_data_comparer);
            };
    std::ranges::for_each(meaningful_results, sort_for_left_value);
}

inline UpperSetIndex CreateUpperSetIndex(
        EnumeratedMeaningfulDataResults meaningful_results,
        std::vector<RecordClassifierValueId> const& rhs_rcv_ids_selection) {
    DESBORDANTE_ASSUME(!rhs_rcv_ids_selection.empty());
    // First value of rhs_rcv_ids_selection is intended to be the value ID of the total
    // classifier.
    assert(rhs_rcv_ids_selection.front() == kLowestRCValueId);
    SortForAllLeftValues(meaningful_results);
    UpperSetIndex upper_set_index;
    std::size_t const lt_pvalue_number = meaningful_results.size();
    upper_set_index.reserve(lt_pvalue_number);
    for (auto const& meaningful_left_pvalue_results : meaningful_results) {
        if (meaningful_left_pvalue_results.empty()) [[unlikely]] {
            // All comparisons yielded the total decision boundary.
            upper_set_index.emplace_back();
            continue;
        }
        auto end_lhs_rcv_iter = --rhs_rcv_ids_selection.crend(),
             current_lhs_rcv_iter = rhs_rcv_ids_selection.crbegin();
        // clang-tidy can't tell that dec_until_le initializes this variable if left uninitialized
        // here but gcc optimizes this initialization away.
        RecordClassifierValueId current_rcv_id{};
        auto advance_until_leq = [&current_rcv_id, &end_lhs_rcv_iter,
                                  &current_lhs_rcv_iter](RecordClassifierValueId rcv_id) {
            for (; current_lhs_rcv_iter != end_lhs_rcv_iter; ++current_lhs_rcv_iter) {
                if ((current_rcv_id = *current_lhs_rcv_iter) <= rcv_id) return;
            }
        };
        // Relying on sort order here, first value is the greatest.
        advance_until_leq(meaningful_left_pvalue_results.front().comparison_result);
        if (current_lhs_rcv_iter == end_lhs_rcv_iter) {
            // If the first value is too small, then so are all the others.
            upper_set_index.emplace_back();
            continue;
        }
        // TODO: subtract results that are not included.
        LTPVComparisonOrderedRTPValueIDs rt_pvalue_ids =
                util::GetPreallocatedVector<PartitionValueId>(
                        meaningful_left_pvalue_results.size());
        std::size_t record_set_cardinality = 0;
        std::vector<std::pair<RecordClassifierValueId, UpperSetCardinalities>>
                cardinality_map_entries;

        for (auto const& [rcv_id, rt_pvalue_id, cluster_size] : meaningful_left_pvalue_results) {
            if (rcv_id < current_rcv_id) {
                cardinality_map_entries.emplace_back(
                        std::distance(current_lhs_rcv_iter, end_lhs_rcv_iter),
                        UpperSetCardinalities{record_set_cardinality, rt_pvalue_ids.size()});
                ++current_lhs_rcv_iter;
                advance_until_leq(rcv_id);
                if (current_lhs_rcv_iter == end_lhs_rcv_iter) goto end_loop;
            }
            rt_pvalue_ids.push_back(rt_pvalue_id);
            assert(cluster_size != 0);
            record_set_cardinality += cluster_size;
        }
        cardinality_map_entries.emplace_back(
                std::distance(current_lhs_rcv_iter, end_lhs_rcv_iter),
                UpperSetCardinalities{record_set_cardinality, rt_pvalue_ids.size()});
    end_loop:
        std::ranges::reverse(cardinality_map_entries);
        LTPValueRCVIDUpperSetCardinalityMap cardinality_map{boost::container::ordered_unique_range,
                                                            cardinality_map_entries.begin(),
                                                            cardinality_map_entries.end()};
        upper_set_index.emplace_back(std::move(rt_pvalue_ids), std::move(cardinality_map));
    }
    return upper_set_index;
}

// For symmetrical comparison functions. Assumes that the left and the right values were the same
// and only the values following the value on the left were compared. Assumes that if equal values
// were compared, they are at the beginning of each row. Fills in the rest of value pair comparison
// results.
inline void SymmetricClosure(EnumeratedMeaningfulDataResults& enumerated) {
    std::size_t const left_values = enumerated.size();
    for (PartitionValueId left_pvalue_id : utility::IndexRange(left_values)) {
        MeaningfulLeftValueResults<RecordClassifierValueId> const& left_pvalue_results =
                enumerated[left_pvalue_id];
        if (left_pvalue_results.empty()) continue;
        for (auto const& [rcv_id, rt_pvalue_id, cluster_size] :
             std::ranges::drop_view{left_pvalue_results,
                                    left_pvalue_results.front().rt_pvalue_id == left_pvalue_id}) {
            // could be <=, but the == check is done above, see CalcForSame for !EqMax case
            if (rt_pvalue_id < left_pvalue_id) break;
            auto& left_pvalue_eq_to_right_pvalue_results = enumerated[rt_pvalue_id];
            left_pvalue_eq_to_right_pvalue_results.emplace_back(rcv_id, left_pvalue_id,
                                                                cluster_size);
        }
    }
}

template <typename DecisionBoundaryType, typename ResultType>
ComponentHandlingInfo BuildIndexes(EnumeratedMeaningfulDataResults enumerated,
                                   std::vector<ResultType> comparison_results,
                                   rcv_id_selectors::Selector<ResultType> const& selector,
                                   bool total_decision_boundary_is_universal,
                                   ComponentStructureAssertions structure_assertions) {
    auto lhs_indices = selector.GetSubsetIndices(comparison_results);
    SearchSpaceComponentSpecification ss_component{
            {ToDecisionBoundaries<DecisionBoundaryType>(std::move(comparison_results)),
             total_decision_boundary_is_universal},
            lhs_indices};

    ValueMatrix value_matrix = CreateValueMatrix(enumerated);

    std::vector<RecordClassifierValueId> const& lhs_rcv_ids =
            ss_component.rcv_id_lr_map.lhs_to_rhs_map;
    DESBORDANTE_ASSUME(!lhs_rcv_ids.empty());
    if (lhs_rcv_ids.size() <= 1)
        return {std::move(ss_component), {std::move(value_matrix), {}}, structure_assertions};

    UpperSetIndex upper_set_index = CreateUpperSetIndex(std::move(enumerated), lhs_rcv_ids);

    return {std::move(ss_component),
            {std::move(value_matrix), std::move(upper_set_index)},
            structure_assertions};
}
}  // namespace algos::hymde::record_match_indexes::calculators
