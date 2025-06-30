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
inline void SortForAllLeftValues(EnumeratedMeaningfulDataResults& meaningful_results) {
    auto result_data_comparer = [](MeaningfulResult<RecordClassifierValueId> const& p1,
                                   MeaningfulResult<RecordClassifierValueId> const& p2) {
        auto const& [rcv_id1, pvid1] = p1;
        auto const& [rcv_id2, pvid2] = p2;
        return rcv_id1 > rcv_id2 || (rcv_id1 == rcv_id2 && pvid1 < pvid2);
    };
    auto sort_for_left_value =
            [&result_data_comparer](
                    std::pair<std::vector<std::pair<RecordClassifierValueId, PartitionValueId>>,
                              std::size_t>& left_value_results) {
                std::ranges::sort(left_value_results.first, result_data_comparer);
            };
    std::ranges::for_each(meaningful_results, sort_for_left_value);
}

inline ValueMatrix CreateValueMatrix(EnumeratedMeaningfulDataResults const& meaningful_results) {
    ValueMatrix value_matrix;
    std::size_t const left_value_number = meaningful_results.size();
    value_matrix.reserve(left_value_number);
    for (auto const& [left_value_results, _] : meaningful_results) {
        ValueMatrixRow& left_value_row = value_matrix.emplace_back(left_value_results.size());
        for (auto const& [rcv_id, right_partition_value_id] : left_value_results) {
            // Should have been excluded during enumeration.
            assert(rcv_id != kLowestRCValueId);
            left_value_row.try_emplace(right_partition_value_id, rcv_id);
        }
    }
    return value_matrix;
}

inline UpperSetIndex CreateUpperSetIndex(EnumeratedMeaningfulDataResults const& meaningful_results,
                                         std::vector<RecordClassifierValueId> const& lhs_rcv_ids,
                                         PartitionIndex::PositionListIndex const& right_pli) {
    UpperSetIndex upper_set_index;
    std::size_t const value_number = meaningful_results.size();
    upper_set_index.reserve(value_number);
    for (auto const& [meaningful_left_value_results, meaningful_records_count] :
         meaningful_results) {
        if (meaningful_left_value_results.empty()) [[unlikely]] {
            // All comparisons yielded the total decision boundary.
            upper_set_index.emplace_back();
            assert(meaningful_records_count == 0);
            continue;
        }
        DESBORDANTE_ASSUME(!lhs_rcv_ids.empty());
        // First value of lhs_rcv_ids is intended to be the value ID of the total classifier.
        assert(lhs_rcv_ids.front() == kLowestRCValueId);
        auto end = --lhs_rcv_ids.crend(), current = lhs_rcv_ids.crbegin();
        // clang-tidy can't tell that dec_until_le initializes this variable if left uninitialized
        // here but gcc optimizes this initialization away.
        RecordClassifierValueId current_rcv_id{};
        auto advance_until_leq = [&current_rcv_id, &end, &current](RecordClassifierValueId value) {
            for (; current != end; ++current) {
                if ((current_rcv_id = *current) <= value) return;
            }
        };
        // Relying on sort order here, first value is the greatest.
        advance_until_leq(meaningful_left_value_results.front().first);
        if (current == end) {
            // If the first value is too small, then so are all the others.
            upper_set_index.emplace_back();
            continue;
        }
        EndIdMap end_id_map;
        std::vector<RecordIdentifier> meaningful_records;
        meaningful_records.reserve(meaningful_records_count);
        for (auto const& [rcv_id, partition_value_id] : meaningful_left_value_results) {
            if (rcv_id < current_rcv_id) {
                end_id_map.try_emplace(end_id_map.end(), std::distance(current, end),
                                       meaningful_records.size());
                ++current;
                advance_until_leq(rcv_id);
                if (current == end) goto end_loop;
            }
            PartitionIndex::RecordCluster const& cluster = right_pli[partition_value_id];
            meaningful_records.insert(meaningful_records.end(), cluster.begin(), cluster.end());
        }
        end_id_map.try_emplace(end_id_map.end(), std::distance(current, end),
                               meaningful_records.size());
    end_loop:
        upper_set_index.emplace_back(
                FlatUpperSetIndex{std::move(meaningful_records), std::move(end_id_map)});
    }
    return upper_set_index;
}

// For symmetrical comparison functions. Assumes that the left and the right values were the same
// and only the values following the value on the left were compared. Assumes that if equal values
// were compared, they are at the beginning of each row. Fills in the rest of value pair comparison
// results.
inline void SymmetricClosure(EnumeratedMeaningfulDataResults& enumerated,
                             PartitionIndex::PositionListIndex const& right_pli) {
    std::size_t const left_values = enumerated.size();
    for (PartitionValueId left_pvalue_id : utility::IndexRange(left_values)) {
        MeaningfulLeftValueResults<RecordClassifierValueId> const& left_pvalue_results =
                enumerated[left_pvalue_id].first;
        if (left_pvalue_results.empty()) continue;
        for (auto const& [rcv_id, right_pvalue_id] : std::ranges::drop_view{
                     left_pvalue_results, left_pvalue_results.front().second == left_pvalue_id}) {
            // could be <=, but the == check is done above, see CalcForSame for !EqMax case
            if (right_pvalue_id < left_pvalue_id) break;
            auto& [left_pvalue_eq_to_right_pvalue_results, meaningful_records_count] =
                    enumerated[right_pvalue_id];
            left_pvalue_eq_to_right_pvalue_results.emplace_back(rcv_id, left_pvalue_id);
            meaningful_records_count += right_pli[left_pvalue_id].size();
        }
    }
}

template <typename DecisionBoundaryType, typename ResultType>
ComponentHandlingInfo BuildIndexes(EnumeratedMeaningfulDataResults enumerated,
                                   std::vector<ResultType> comparison_results,
                                   PartitionIndex::PositionListIndex const& right_pli,
                                   rcv_id_selectors::Selector<ResultType> const& selector,
                                   bool total_decision_boundary_is_universal,
                                   ComponentStructureAssertions structure_assertions) {
    SortForAllLeftValues(enumerated);

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

    UpperSetIndex upper_set_index = CreateUpperSetIndex(enumerated, lhs_rcv_ids, right_pli);

    return {std::move(ss_component),
            {std::move(value_matrix), std::move(upper_set_index)},
            structure_assertions};
}
}  // namespace algos::hymde::record_match_indexes::calculators
