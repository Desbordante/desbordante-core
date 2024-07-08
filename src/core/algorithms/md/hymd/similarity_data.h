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
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/index.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class SimilarityData {
public:
    using ColMatchesInfo = std::vector<
            std::tuple<std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure>,
                       model::Index, model::Index>>;

private:
    indexes::RecordsInfo const* const records_info_;
    bool const single_table_;

    std::vector<ColumnMatchInfo> const column_matches_sim_info_;
    std::vector<std::vector<ColumnClassifierValueId>> const column_matches_lhs_ids_;

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return records_info_->GetLeftCompressor();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return records_info_->GetRightCompressor();
    }

public:
    SimilarityData(
            indexes::RecordsInfo* records_info,
            std::vector<ColumnMatchInfo> column_matches_sim_info,
            std::vector<std::vector<ColumnClassifierValueId>> column_matches_lhs_ids) noexcept
        : records_info_(records_info),
          single_table_(records_info_->OneTableGiven()),
          column_matches_sim_info_(std::move(column_matches_sim_info)),
          column_matches_lhs_ids_(std::move(column_matches_lhs_ids)) {}

    static SimilarityData CreateFrom(indexes::RecordsInfo* records_info,
                                     ColMatchesInfo column_matches_info,
                                     util::WorkerThreadPool& pool);

    [[nodiscard]] std::size_t GetColumnMatchNumber() const noexcept {
        return column_matches_sim_info_.size();
    }

    [[nodiscard]] std::vector<ColumnClassifierValueId> CreateMaxRhs() const noexcept {
        std::vector<ColumnClassifierValueId> max_rhs;
        max_rhs.reserve(GetColumnMatchNumber());
        for (ColumnMatchInfo const& cm_info : column_matches_sim_info_) {
            max_rhs.push_back(cm_info.similarity_info.classifier_values.size() - 1);
        }
        return max_rhs;
    }

    std::vector<std::vector<ColumnClassifierValueId>> const& GetLhsIds() const noexcept {
        return column_matches_lhs_ids_;
    }

    [[nodiscard]] std::vector<ColumnMatchInfo> const& GetColumnMatchesInfo() const noexcept {
        return column_matches_sim_info_;
    }

    [[nodiscard]] std::pair<model::Index, model::Index> GetColMatchIndices(
            model::Index index) const {
        auto const& [_, left_column_index, right_column_index] = column_matches_sim_info_[index];
        return {left_column_index, right_column_index};
    }

    [[nodiscard]] std::size_t GetLeftSize() const noexcept {
        return GetLeftCompressor().GetNumberOfRecords();
    }

    [[nodiscard]] std::unordered_set<PairComparisonResult> CompareAllWith(
            RecordIdentifier left_record_id) const;
    [[nodiscard]] PairComparisonResult CompareRecords(CompressedRecord const& left_record,
                                                      CompressedRecord const& right_record) const;

    [[nodiscard]] model::md::DecisionBoundary GetLhsDecisionBoundary(
            model::Index column_match_index,
            ColumnClassifierValueId classifier_value_id) const noexcept;

    [[nodiscard]] model::md::DecisionBoundary GetDecisionBoundary(
            model::Index column_match_index,
            ColumnClassifierValueId classifier_value_id) const noexcept;
};

}  // namespace algos::hymd
