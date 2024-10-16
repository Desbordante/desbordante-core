#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/utility/index_range.h"
#include "algorithms/md/hymd/utility/make_unique_for_overwrite.h"
#include "model/index.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymd {

class SimilarityData::Creator {
    indexes::RecordsInfo* const records_info_;
    Measures const& measures_;
    util::WorkerThreadPool* pool_ptr_;

    void ProcessMeasureIndexes(model::Index column_match_index,
                               std::vector<ColumnMatchInfo>& column_matches_info,
                               std::vector<LhsCCVIdsInfo>& all_lhs_ccv_ids_info,
                               std::vector<bool>& short_sampling_enable,
                               std::vector<std::pair<model::md::DecisionBoundary, model::Index>>&
                                       trivial_column_matches_info,
                               std::vector<model::Index>& non_trivial_indices) {
        MeasurePtr const& measure = measures_[column_match_index];
        auto [left_col_index, right_col_index] = measure->GetIndices();
        auto [lhs_ccv_id_info, indexes] = measure->MakeIndexes(pool_ptr_, *records_info_);
        bool const is_trivial = indexes.classifier_values.size() == 1;
        if (is_trivial) {
            // These column matches are excluded from the normal operations and accounted for at the
            // end of the algorithm. They are "trivial" because we immediately know all MDs in
            // the answer that include them in either RHS or LHS column classifiers with non-zero
            // decision boundaries: empty LHS + RHS with each boundary that is not 0.0.
            // Excluding them also prevents a situation during LHS specialization where their
            // corresponding CCV IDs (indices in the list of natural decision boundaries) become 1
            // and immediately get rejected for every single specialization procedure call.
            trivial_column_matches_info.emplace_back(indexes.classifier_values.front(),
                                                     column_match_index);
        } else {
            non_trivial_indices.push_back(column_match_index);
            column_matches_info.emplace_back(std::move(indexes), left_col_index, right_col_index);
            all_lhs_ccv_ids_info.push_back(std::move(lhs_ccv_id_info));
            short_sampling_enable.push_back(measure->IsSymmetricalAndEqIsMax());
        }
    }

    std::unique_ptr<model::Index[]> CreateArrangement(
            std::vector<LhsCCVIdsInfo> const& all_lhs_ccv_ids_info) {
        std::size_t const non_trivial_number = all_lhs_ccv_ids_info.size();
        auto arrangement_ptr = utility::MakeUniqueForOverwrite<model::Index[]>(non_trivial_number);
        auto start = arrangement_ptr.get(), end = start + non_trivial_number;
        std::iota(start, end, 0);
        // Sort based on the number of LHS CCV IDs. This should help reduce lattice memory use by
        // decreasing the number of LHS child node arrays of large size assuming dependencies
        // involving column matches with a lot of LHS decision boundaies are numerous enough.
        // The idea is this: imagine there is a column match with 1 decision boundary and a column
        // match with 300 decision boundaries and there are 30 column matches inbetween. The nodes
        // representing a column classifier with a column match that has a lot of decision
        // boundaries, which are more likely to be numerous, will have 0 elements in their child
        // array, while the nodes storing a child node array with 31 elements will be few in number,
        // which should save space.
        std::sort(start, end, [&](model::Index i, model::Index j) {
            std::size_t const lhs_ccv_ids1 = all_lhs_ccv_ids_info[i].lhs_to_rhs_map.size();
            std::size_t const lhs_ccv_ids2 = all_lhs_ccv_ids_info[j].lhs_to_rhs_map.size();
            return lhs_ccv_ids1 < lhs_ccv_ids2 || (lhs_ccv_ids1 == lhs_ccv_ids2 && i < j);
        });
        return arrangement_ptr;
    }

public:
    using PreprocessingResult =
            std::tuple<std::vector<ColumnMatchInfo>, std::vector<LhsCCVIdsInfo>,
                       std::vector<std::pair<model::md::DecisionBoundary, model::Index>>,
                       std::vector<bool>, std::vector<model::Index>>;

    Creator(indexes::RecordsInfo* const records_info, Measures const& measures,
            util::WorkerThreadPool* pool_ptr)
        : records_info_(records_info), measures_(measures), pool_ptr_(pool_ptr) {}

    PreprocessingResult CalculateIndexes() {
        std::size_t const col_match_number = measures_.size();
        std::vector<ColumnMatchInfo> column_matches_info =
                util::GetPreallocatedVector<ColumnMatchInfo>(col_match_number);
        std::vector<LhsCCVIdsInfo> all_lhs_ccv_ids_info =
                util::GetPreallocatedVector<LhsCCVIdsInfo>(col_match_number);
        std::vector<bool> short_sampling_enable =
                util::GetPreallocatedVector<bool>(col_match_number);
        std::vector<std::pair<model::md::DecisionBoundary, model::Index>>
                trivial_column_matches_info;
        std::vector<model::Index> non_trivial_indices =
                util::GetPreallocatedVector<model::Index>(col_match_number);

        for (model::Index column_match_index : utility::IndexRange(col_match_number)) {
            ProcessMeasureIndexes(column_match_index, column_matches_info, all_lhs_ccv_ids_info,
                                  short_sampling_enable, trivial_column_matches_info,
                                  non_trivial_indices);
        }

        return {std::move(column_matches_info), std::move(all_lhs_ccv_ids_info),
                std::move(trivial_column_matches_info), std::move(short_sampling_enable),
                std::move(non_trivial_indices)};
    }

    void Reorder(PreprocessingResult& preprocessing_result) {
        auto& [column_matches_info, all_lhs_ccv_ids_info, _, short_sampling_enable,
               non_trivial_indices] = preprocessing_result;

        std::unique_ptr<model::Index[]> arrangement = CreateArrangement(all_lhs_ccv_ids_info);

        std::size_t const non_trivial_number = all_lhs_ccv_ids_info.size();
        std::vector<ColumnMatchInfo> sorted_column_matches_info =
                util::GetPreallocatedVector<ColumnMatchInfo>(non_trivial_number);
        std::vector<LhsCCVIdsInfo> sorted_all_lhs_ccv_ids_info =
                util::GetPreallocatedVector<LhsCCVIdsInfo>(non_trivial_number);
        std::vector<model::Index> sorted_non_trivial_indices =
                util::GetPreallocatedVector<model::Index>(non_trivial_number);
        std::vector<bool> sorted_short_sampling_enable =
                util::GetPreallocatedVector<bool>(non_trivial_number);

        for (model::Index non_trivial_index : std::span{arrangement.get(), non_trivial_number}) {
            sorted_column_matches_info.push_back(std::move(column_matches_info[non_trivial_index]));
            sorted_all_lhs_ccv_ids_info.push_back(
                    std::move(all_lhs_ccv_ids_info[non_trivial_index]));
            sorted_non_trivial_indices.push_back(non_trivial_indices[non_trivial_index]);
            sorted_short_sampling_enable.push_back(short_sampling_enable[non_trivial_index]);
        }

        column_matches_info = std::move(sorted_column_matches_info);
        all_lhs_ccv_ids_info = std::move(sorted_all_lhs_ccv_ids_info);
        non_trivial_indices = std::move(sorted_non_trivial_indices);
        short_sampling_enable = std::move(sorted_short_sampling_enable);
    }
};

std::pair<SimilarityData, std::vector<bool>> SimilarityData::CreateFrom(
        indexes::RecordsInfo* const records_info, Measures const& measures,
        util::WorkerThreadPool* pool_ptr) {
    Creator creator{records_info, measures, pool_ptr};

    Creator::PreprocessingResult result = creator.CalculateIndexes();
    creator.Reorder(result);

    auto& [column_matches_info, all_lhs_ccv_ids_info, trivial_column_matches_info,
           short_sampling_enable, non_trivial_indices] = result;

    return {{records_info, std::move(column_matches_info), std::move(all_lhs_ccv_ids_info),
             std::move(non_trivial_indices), std::move(trivial_column_matches_info)},
            std::move(short_sampling_enable)};
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

}  // namespace algos::hymd
