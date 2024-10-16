#pragma once

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "algorithms/md/hymd/utility/invalidated_rhss.h"
#include "model/index.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class Validator {
public:
    using OneRhsRecommendations = std::vector<Recommendation>;
    using AllRhsRecommendations = std::vector<OneRhsRecommendations>;

    struct Result {
        AllRhsRecommendations all_rhs_recommendations;
        utility::InvalidatedRhss invalidated;
        bool is_unsupported;
    };

private:
    template <typename PairProvider>
    class SetPairProcessor;
    class OneCardPairProvider;
    class MultiCardPairProvider;

    struct WorkingInfo {
        OneRhsRecommendations* recommendations;
        MdElement old_rhs;
        ColumnClassifierValueId current_ccv_id;
        std::size_t col_match_values;
        ColumnClassifierValueId interestingness_id;
        indexes::CompressedRecords const* right_records;
        indexes::SimilarityMatrix const* similarity_matrix;
        model::Index left_index;
        model::Index right_index;

        bool EnoughRecommendations() const {
            return true;
            // <=> return recommendations.size() >= 1 /* was 20 */;
            // I believe this check is no longer needed, as we are only giving "useful"
            // recommendations, which means those that are very likely to actually remove the need
            // to validate MDs.
        }

        bool ShouldStop() const {
            return current_ccv_id == kLowestCCValueId && EnoughRecommendations();
        }

        WorkingInfo(MdElement old_rhs, OneRhsRecommendations& recommendations,
                    std::size_t col_match_values, ColumnClassifierValueId interestingness_id,
                    indexes::CompressedRecords const& right_records,
                    indexes::SimilarityMatrix const& similarity_matrix,
                    model::Index const left_index, model::Index const right_index)
            : recommendations(&recommendations),
              old_rhs(old_rhs),
              current_ccv_id(old_rhs.ccv_id),
              col_match_values(col_match_values),
              interestingness_id(interestingness_id),
              right_records(&right_records),
              similarity_matrix(&similarity_matrix),
              left_index(left_index),
              right_index(right_index) {}
    };

    std::vector<std::vector<WorkingInfo>> current_working_;
    std::vector<Result> results_;
    util::WorkerThreadPool* pool_;
    indexes::RecordsInfo const* const records_info_;
    std::vector<ColumnMatchInfo> const* const column_matches_info_;
    std::size_t const min_support_;
    lattice::MdLattice* const lattice_;

    [[nodiscard]] bool Supported(std::size_t support) const noexcept {
        return support >= min_support_;
    }

    [[nodiscard]] model::Index GetLeftPliIndex(model::Index const column_match_index) const {
        return (*column_matches_info_)[column_match_index].left_column_index;
    }

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return records_info_->GetLeftCompressor();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return records_info_->GetRightCompressor();
    }

    std::size_t GetLeftValueNum(model::Index const col_match_index) const {
        return GetLeftCompressor().GetPli(GetLeftPliIndex(col_match_index)).GetClusters().size();
    }

    std::size_t GetTotalPairsNum() const noexcept {
        return records_info_->GetTotalPairsNum();
    }

    [[nodiscard]] indexes::RecSet const* GetSimilarRecords(ValueIdentifier value_id,
                                                           model::Index lhs_ccv_id,
                                                           model::Index column_match_index) const;
    void MakeWorkingAndRecs(lattice::ValidationInfo const& info, std::vector<WorkingInfo>& working,
                            AllRhsRecommendations& recommendations);
    void Initialize(std::vector<lattice::ValidationInfo>& validation_info);

public:
    Validator(util::WorkerThreadPool* pool, indexes::RecordsInfo const* records_info,
              std::vector<ColumnMatchInfo> const& column_matches_info, std::size_t min_support,
              lattice::MdLattice* lattice)
        : pool_(pool),
          records_info_(records_info),
          column_matches_info_(&column_matches_info),
          min_support_(min_support),
          lattice_(lattice) {}

    std::vector<Result> const& ValidateAll(std::vector<lattice::ValidationInfo>& validation_info);
    void Validate(lattice::ValidationInfo& validation_info, Result& result,
                  std::vector<WorkingInfo>& working) const;
};

}  // namespace algos::hymd
