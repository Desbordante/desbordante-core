#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"

namespace {
struct IndexInfo {
    std::size_t lhs_ccv_id_number;
    std::size_t rhs_ccv_id_number;
    model::Index original;
    model::Index non_trivial;

    bool operator<(IndexInfo const& other) {
        return lhs_ccv_id_number < other.lhs_ccv_id_number ||
               (lhs_ccv_id_number == other.lhs_ccv_id_number && original < other.original);
    }
};
}  // namespace

namespace algos::hymd {

std::pair<SimilarityData, std::vector<bool>> SimilarityData::CreateFrom(
        indexes::RecordsInfo* const records_info, MeasureCreators const& measure_creators,
        std::shared_ptr<RelationalSchema> const& left_schema,
        std::shared_ptr<RelationalSchema> const& right_schema, util::WorkerThreadPool* pool_ptr) {
    bool const one_table_given = records_info->OneTableGiven();
    std::size_t const col_match_number = measure_creators.size();
    std::vector<bool> short_sampling_enable;
    short_sampling_enable.reserve(col_match_number);
    std::vector<ColumnMatchInfo> column_matches_info;
    column_matches_info.reserve(col_match_number);
    std::vector<IndexInfo> col_match_index_info;
    col_match_index_info.reserve(col_match_number);
    std::vector<LhsCCVIdsInfo> all_lhs_ccv_ids_info;
    all_lhs_ccv_ids_info.reserve(col_match_number);
    std::vector<std::pair<TrivialColumnMatchInfo, model::Index>> trivial_column_matches_info;
    auto const& left_records = records_info->GetLeftCompressor();
    auto const& right_records = records_info->GetRightCompressor();
    model::Index column_match_index = 0;
    model::Index non_trivial_column_match_index = 0;
    for (std::shared_ptr<SimilarityMeasureCreator> const& creator : measure_creators) {
        auto const [left_col_index, right_col_index] =
                creator->GetIndices(*left_schema, *right_schema);
        std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure> measure =
                creator->MakeMeasure(pool_ptr);
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
        auto [lhs_ccv_id_info, indexes] = measure->MakeIndexes(
                std::move(data_info_left), std::move(data_info_right), right_pli.GetClusters());
        IndexInfo& last_index_info = col_match_index_info.emplace_back(
                lhs_ccv_id_info.lhs_to_rhs_map.size(), indexes.classifier_values.size(),
                column_match_index);
        if (indexes.classifier_values.size() != 1) {
            last_index_info.non_trivial = non_trivial_column_match_index++;
            column_matches_info.emplace_back(std::move(indexes), left_col_index, right_col_index);
            all_lhs_ccv_ids_info.push_back(std::move(lhs_ccv_id_info));
            short_sampling_enable.push_back(measure->IsSymmetricalAndEqIsMax());
        } else {
            TrivialColumnMatchInfo trivial_column_match_info = {indexes.classifier_values.front(),
                                                                left_col_index, right_col_index};
            trivial_column_matches_info.emplace_back(std::move(trivial_column_match_info),
                                                     column_match_index);
        }
        ++column_match_index;
    }
    std::sort(col_match_index_info.begin(), col_match_index_info.end());
    std::vector<model::Index> sorted_to_original;
    sorted_to_original.reserve(col_match_number);
    std::size_t non_trivial_col_match_number = column_matches_info.size();
    std::vector<ColumnMatchInfo> sorted_column_matches_info;
    sorted_column_matches_info.reserve(non_trivial_col_match_number);
    std::vector<LhsCCVIdsInfo> sorted_all_lhs_ccv_ids_info;
    sorted_all_lhs_ccv_ids_info.reserve(non_trivial_col_match_number);
    std::vector<bool> sorted_short_sampling_enable;
    sorted_short_sampling_enable.reserve(non_trivial_col_match_number);
    for (auto const& [lhs_ccv_id_number, rhs_ccv_id_number, index, non_trivial_index] :
         col_match_index_info) {
        if (rhs_ccv_id_number != 1) {
            sorted_to_original.push_back(index);
            sorted_column_matches_info.push_back(std::move(column_matches_info[non_trivial_index]));
            sorted_all_lhs_ccv_ids_info.push_back(
                    std::move(all_lhs_ccv_ids_info[non_trivial_index]));
            sorted_short_sampling_enable.push_back(short_sampling_enable[non_trivial_index]);
        }
    }
    return {{records_info, std::move(sorted_column_matches_info),
             std::move(sorted_all_lhs_ccv_ids_info), std::move(sorted_to_original),
             std::move(trivial_column_matches_info)},
            std::move(sorted_short_sampling_enable)};
}

model::md::DecisionBoundary SimilarityData::GetLhsDecisionBoundary(
        model::Index column_match_index,
        ColumnClassifierValueId classifier_value_id) const noexcept {
    return column_matches_sim_info_[column_match_index].similarity_info.classifier_values
            [column_matches_lhs_ids_info_[column_match_index].lhs_to_rhs_map[classifier_value_id]];
}

model::md::DecisionBoundary SimilarityData::GetDecisionBoundary(
        model::Index column_match_index,
        ColumnClassifierValueId classifier_value_id) const noexcept {
    return column_matches_sim_info_[column_match_index]
            .similarity_info.classifier_values[classifier_value_id];
}

model::md::DecisionBoundary SimilarityData::GetTrivialDecisionBoundary(
        model::Index trivial_column_match_index) const noexcept {
    return trivial_column_matches_info_[trivial_column_match_index].first.similarity;
}

model::Index SimilarityData::GetTrivialColumnMatchIndex(
        model::Index trivial_column_match_index) const noexcept {
    return trivial_column_matches_info_[trivial_column_match_index].second;
}

}  // namespace algos::hymd
