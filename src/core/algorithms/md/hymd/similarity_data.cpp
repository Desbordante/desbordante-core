#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"

namespace algos::hymd {

SimilarityData SimilarityData::CreateFrom(indexes::RecordsInfo* const records_info,
                                          ColMatchesInfo const column_matches_info_initial) {
    bool const one_table_given = records_info->OneTableGiven();
    std::size_t const col_match_number = column_matches_info_initial.size();
    std::vector<ColumnMatchInfo> column_matches_info;
    column_matches_info.reserve(col_match_number);
    std::vector<std::vector<model::Index>> column_matches_lhs_ids;
    column_matches_lhs_ids.reserve(col_match_number);
    auto const& left_records = records_info->GetLeftCompressor();
    auto const& right_records = records_info->GetRightCompressor();
    for (auto const& [measure, left_col_index, right_col_index] : column_matches_info_initial) {
        auto const& left_pli = left_records.GetPli(left_col_index);
        // TODO: cache DataInfo.
        std::shared_ptr<preprocessing::DataInfo const> data_info_left =
                preprocessing::DataInfo::MakeFrom(left_pli, measure->GetArgType());
        std::shared_ptr<preprocessing::DataInfo const> data_info_right;
        auto const& right_pli = right_records.GetPli(right_col_index);
        if (one_table_given && left_col_index == right_col_index) {
            data_info_right = data_info_left;
        } else {
            data_info_right = preprocessing::DataInfo::MakeFrom(right_pli, measure->GetArgType());
        }
        // TODO: sort column matches on the number of LHS CCV IDs.
        auto [lhs_ccv_ids, indexes] = measure->MakeIndexes(
                std::move(data_info_left), std::move(data_info_right), right_pli.GetClusters());
        column_matches_info.emplace_back(std::move(indexes), left_col_index, right_col_index);
        column_matches_lhs_ids.push_back(std::move(lhs_ccv_ids));
    }
    return {records_info, std::move(column_matches_info), std::move(column_matches_lhs_ids)};
}

model::md::DecisionBoundary SimilarityData::GetLhsDecisionBoundary(
        model::Index column_match_index,
        ColumnClassifierValueId classifier_value_id) const noexcept {
    return column_matches_sim_info_[column_match_index]
            .similarity_info
            .classifier_values[column_matches_lhs_ids_[column_match_index][classifier_value_id]];
}

model::md::DecisionBoundary SimilarityData::GetDecisionBoundary(
        model::Index column_match_index,
        ColumnClassifierValueId classifier_value_id) const noexcept {
    return column_matches_sim_info_[column_match_index]
            .similarity_info.classifier_values[classifier_value_id];
}

}  // namespace algos::hymd
