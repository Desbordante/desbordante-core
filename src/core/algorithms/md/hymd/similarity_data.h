#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lhs_ccv_ids_info.h"
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/index.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class SimilarityData {
public:
    using MeasurePtr = std::shared_ptr<preprocessing::similarity_measure::SimilarityMeasure>;
    using Measures = std::vector<MeasurePtr>;

private:
    indexes::RecordsInfo const* const records_info_;

    std::vector<ColumnMatchInfo> const column_matches_sim_info_;
    std::vector<LhsCCVIdsInfo> const column_matches_lhs_ids_info_;

    std::vector<model::Index> const sorted_to_original_;
    std::vector<std::pair<TrivialColumnMatchInfo, model::Index>> trivial_column_matches_info_;

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return records_info_->GetLeftCompressor();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return records_info_->GetRightCompressor();
    }

public:
    SimilarityData(indexes::RecordsInfo* records_info,
                   std::vector<ColumnMatchInfo> column_matches_sim_info,
                   std::vector<LhsCCVIdsInfo> column_matches_lhs_ids_info,
                   std::vector<model::Index> sorted_to_original,
                   std::vector<std::pair<TrivialColumnMatchInfo, model::Index>>
                           trivial_column_matches_info) noexcept
        : records_info_(records_info),
          column_matches_sim_info_(std::move(column_matches_sim_info)),
          column_matches_lhs_ids_info_(std::move(column_matches_lhs_ids_info)),
          sorted_to_original_(std::move(sorted_to_original)),
          trivial_column_matches_info_(std::move(trivial_column_matches_info)) {}

    static std::pair<SimilarityData, std::vector<bool>> CreateFrom(
            indexes::RecordsInfo* records_info, Measures const& measures,
            util::WorkerThreadPool* pool_ptr);

    [[nodiscard]] std::size_t GetColumnMatchNumber() const noexcept {
        return column_matches_sim_info_.size();
    }

    [[nodiscard]] std::size_t GetTrivialColumnMatchNumber() const noexcept {
        return trivial_column_matches_info_.size();
    }

    [[nodiscard]] lattice::Rhs CreateMaxRhs() const noexcept {
        lattice::Rhs max_rhs(GetColumnMatchNumber());
        model::Index i = 0;
        for (ColumnMatchInfo const& cm_info : column_matches_sim_info_) {
            max_rhs.Set(i++, cm_info.similarity_info.classifier_values.size() - 1);
        }
        return max_rhs;
    }

    std::vector<LhsCCVIdsInfo> const& GetLhsIdsInfo() const noexcept {
        return column_matches_lhs_ids_info_;
    }

    [[nodiscard]] std::vector<ColumnMatchInfo> const& GetColumnMatchesInfo() const noexcept {
        return column_matches_sim_info_;
    }

    [[nodiscard]] std::pair<model::Index, model::Index> GetColMatchIndices(
            model::Index index) const {
        auto const& [_, left_column_index, right_column_index] = column_matches_sim_info_[index];
        return {left_column_index, right_column_index};
    }

    [[nodiscard]] std::pair<model::Index, model::Index> GetTrivialColMatchIndices(
            model::Index index) const {
        model::Index const left_column_index =
                trivial_column_matches_info_[index].first.left_column_index;
        model::Index const right_column_index =
                trivial_column_matches_info_[index].first.right_column_index;
        return {left_column_index, right_column_index};
    }

    [[nodiscard]] std::vector<model::Index> const& GetIndexMapping() const noexcept {
        return sorted_to_original_;
    }

    [[nodiscard]] model::md::DecisionBoundary GetLhsDecisionBoundary(
            model::Index column_match_index,
            ColumnClassifierValueId classifier_value_id) const noexcept;

    [[nodiscard]] model::md::DecisionBoundary GetDecisionBoundary(
            model::Index column_match_index,
            ColumnClassifierValueId classifier_value_id) const noexcept;

    [[nodiscard]] model::md::DecisionBoundary GetTrivialDecisionBoundary(
            model::Index trivial_column_match_index) const noexcept;

    [[nodiscard]] model::Index GetTrivialColumnMatchIndex(
            model::Index trivial_column_match_index) const noexcept;
};

}  // namespace algos::hymd
