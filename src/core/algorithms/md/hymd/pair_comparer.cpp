#include "algorithms/md/hymd/pair_comparer.h"

#include <algorithm>

#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"

namespace algos::hymd {
std::unordered_set<PairComparisonResult> PairComparer::SampleNext() {
    // TODO: use the "slim" sim index to fill those instead of lookups in similarity matrices
    CompressedRecord const& left_record =
            records_info_->GetLeftCompressor().GetRecords()[left_record_id_];
    std::unordered_set<PairComparisonResult> comparisons;
    // Optimization not performed in Metanome.
    RecordIdentifier const start_from = short_sampling_ ? left_record_id_ + 1 : 0;
    PairComparisonResult pair_sims;
    pair_sims.reserve(column_match_number_);
    indexes::CompressedRecords const& right_records =
            records_info_->GetRightCompressor().GetRecords();
    std::for_each(right_records.begin() + start_from, right_records.end(), [&](auto const& record) {
        for (auto const& [sim_info, left_col_index, right_col_index] : *column_matches_sim_info_) {
            indexes::SimilarityMatrixRow const& sim_matrix_row =
                    sim_info.similarity_matrix[left_record[left_col_index]];
            auto it = sim_matrix_row.find(record[right_col_index]);
            pair_sims.push_back(it == sim_matrix_row.end() ? kLowestCCValueId : it->second);
        }
        comparisons.insert(pair_sims);
        pair_sims.clear();
    });
    ++left_record_id_;
    return comparisons;
}

PairComparisonResult PairComparer::CompareRecords(CompressedRecord const& left_record,
                                                  CompressedRecord const& right_record) const {
    PairComparisonResult comparison_result;
    comparison_result.reserve(column_match_number_);
    for (auto const& [sim_info, left_col_index, right_col_index] : *column_matches_sim_info_) {
        indexes::SimilarityMatrixRow const& row =
                sim_info.similarity_matrix[left_record[left_col_index]];
        auto sim_it = row.find(right_record[right_col_index]);
        comparison_result.push_back(sim_it == row.end() ? kLowestCCValueId : sim_it->second);
    }
    return comparison_result;
}
}  // namespace algos::hymd
