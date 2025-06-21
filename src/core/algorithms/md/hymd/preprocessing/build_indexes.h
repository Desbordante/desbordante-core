#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ranges>
#include <set>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/preprocessing/valid_table_results.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "util/desbordante_assume.h"

namespace algos::hymd::preprocessing {
inline void SortAllRows(EnumeratedValidTableResults& tr_res) {
    auto pair_comparer = [](ValueResult<ColumnClassifierValueId> const& p1,
                            ValueResult<ColumnClassifierValueId> const& p2) {
        auto const& [ccv_id1, vid1] = p1;
        auto const& [ccv_id2, vid2] = p2;
        return ccv_id1 > ccv_id2 || (ccv_id1 == ccv_id2 && vid1 < vid2);
    };
    auto sort_row =
            [&pair_comparer](
                    std::pair<std::vector<std::pair<ColumnClassifierValueId, ValueIdentifier>>,
                              std::size_t>& row_results) {
                std::sort(row_results.first.begin(), row_results.first.end(), pair_comparer);
            };
    std::for_each(tr_res.begin(), tr_res.end(), sort_row);
}

inline indexes::SimilarityMatrix CreateValueMatrix(EnumeratedValidTableResults const& transformed) {
    indexes::SimilarityMatrix value_matrix;
    std::size_t const value_number = transformed.size();
    value_matrix.reserve(value_number);
    for (auto const& [row_results, _] : transformed) {
        indexes::SimilarityMatrixRow& row = value_matrix.emplace_back(row_results.size());
        for (auto const& [ccv_id, value_id] : row_results) {
            if (ccv_id == kLowestCCValueId) break;
            row.try_emplace(value_id, ccv_id);
        }
    }
    return value_matrix;
}

inline indexes::SimilarityIndex CreateUpperSetRecords(
        EnumeratedValidTableResults const& transformed,
        std::vector<ColumnClassifierValueId> const& lhs_ids,
        std::vector<indexes::PliCluster> const& clusters_right) {
    indexes::SimilarityIndex upper_set_records;
    std::size_t const value_number = transformed.size();
    upper_set_records.reserve(value_number);
    for (auto const& [row_results, valid_records_number] : transformed) {
        if (row_results.empty()) {
            upper_set_records.emplace_back();
            assert(valid_records_number == 0);
            continue;
        }
        DESBORDANTE_ASSUME(!lhs_ids.empty());
        auto end = --lhs_ids.crend(), current = lhs_ids.crbegin();
        // clang-tidy can't tell that dec_until_le initializes this variable if left uninitialized
        // here but gcc optimizes this initialization away.
        ColumnClassifierValueId current_ccv_id{};
        auto dec_until_le = [&current_ccv_id, &end, &current](ColumnClassifierValueId value) {
            for (; current != end; ++current) {
                if ((current_ccv_id = *current) <= value) return;
            }
        };
        dec_until_le(row_results.front().first);
        if (current == end) {
            upper_set_records.emplace_back();
            continue;
        }
        indexes::EndIdMap end_id_map;
        std::vector<RecordIdentifier> valid_records;
        valid_records.reserve(valid_records_number);
        for (auto const& [ccv_id, value_id] : row_results) {
            if (ccv_id < current_ccv_id) {
                end_id_map.try_emplace(end_id_map.end(), current_ccv_id, valid_records.size());
                ++current;
                dec_until_le(ccv_id);
                if (current == end) goto end_loop;
            }
            indexes::PliCluster const& cluster = clusters_right[value_id];
            valid_records.insert(valid_records.end(), cluster.begin(), cluster.end());
        }
        end_id_map.try_emplace(end_id_map.end(), current_ccv_id, valid_records.size());
    end_loop:
        upper_set_records.emplace_back(
                indexes::FlatUpperSetIndex{std::move(valid_records), std::move(end_id_map)});
    }
    return upper_set_records;
}

// For symmetrical similarity measures. Assumes that the left and the right columns were the same
// and only the values following the value on the left were compared. Assumes that if equal values
// were compared, they are at the beginning of each row. Fills in the rest of value pair comparison
// results.
inline void SymmetricClosure(EnumeratedValidTableResults& enumerated,
                             std::vector<indexes::PliCluster> const& clusters_right) {
    std::size_t const enumerated_size = enumerated.size();
    for (ColumnClassifierValueId left_value_id = 0; left_value_id < enumerated_size;
         ++left_value_id) {
        ValidRowResults<ColumnClassifierValueId> const& row_results =
                enumerated[left_value_id].first;
        if (row_results.empty()) continue;
        for (auto const& [ccv_id, right_value_id] :
             std::ranges::drop_view{row_results, row_results.front().second == left_value_id}) {
            if (right_value_id < left_value_id) break;
            auto& [next_val_row_results, valid_records_number] = enumerated[right_value_id];
            next_val_row_results.emplace_back(ccv_id, left_value_id);
            valid_records_number += clusters_right[left_value_id].size();
        }
    }
}

template <typename ResultType>
indexes::ColumnPairMeasurements BuildIndexes(EnumeratedValidTableResults enumerated,
                                             std::vector<ResultType> classifier_values,
                                             std::vector<indexes::PliCluster> const& clusters_right,
                                             auto const& pick_lhs_ccv_ids) {
    SortAllRows(enumerated);

    std::vector<ColumnClassifierValueId> lhs_ccv_ids = pick_lhs_ccv_ids(classifier_values);

    indexes::SimilarityMatrix value_matrix = CreateValueMatrix(enumerated);

    if (lhs_ccv_ids.size() <= 1)
        return {std::move(lhs_ccv_ids),
                {std::move(classifier_values), std::move(value_matrix), {}}};

    indexes::SimilarityIndex upper_set_records =
            CreateUpperSetRecords(enumerated, lhs_ccv_ids, clusters_right);

    return {std::move(lhs_ccv_ids),
            {std::move(classifier_values), std::move(value_matrix), std::move(upper_set_records)}};
}
}  // namespace algos::hymd::preprocessing
