#pragma once

#include <cassert>
#include <cstdint>
#include <list>
#include <queue>
#include <span>
#include <unordered_set>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lhs_ccv_ids_info.h"
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "util/desbordante_assume.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class RecordPairInferrer {
private:
    using Comparisons = std::vector<PairComparisonResult>;

    using RankedRecordsValue = std::vector<RecordIdentifier>;
    using RankedRecordsColumnMatch = std::vector<RankedRecordsValue>;

    class RecordRanker;

    template <bool SampleShort>
    class Sampler;

    class InternalConstructToken {
        friend RecordPairInferrer;
        InternalConstructToken() = default;
    };

    // "RHS record" in the comments refers to a record from the right table present in a similarity
    // index.
    class ColumnMatchSamplingInfo {
    private:
        std::size_t column_match_index_;

        // Window size for short sampling on cluster or RHS record index otherwise.
        std::size_t parameter_;

        // Number of pairs where any existing dependency assumed to hold was violated.
        // Possible alternative approach similar to HyFD: number of pairs with a comparison result
        // not encountered before.
        std::size_t num_discovered_ = 0;
        // Number of pairs that were sampled (i.e. selected for comparison).
        std::size_t num_sampled_ = 0;

    public:
        ColumnMatchSamplingInfo(std::size_t column_match_index, std::size_t parameter) noexcept
            : column_match_index_(column_match_index), parameter_(parameter) {}

        // Only used in operator<.
        [[nodiscard]] double CalcEfficiency() const noexcept {
            // Column matches where no record pairs are available for comparison are filtered during
            // RecordPairInferrer construction, after which this number can only increase.
            DESBORDANTE_ASSUME(num_sampled_ != 0);

            return static_cast<double>(num_discovered_) / static_cast<double>(num_sampled_);
        }

        // Used by the sampling queue to compare column matches.
        [[nodiscard]] bool operator<(ColumnMatchSamplingInfo const& other) const noexcept {
            return CalcEfficiency() < other.CalcEfficiency();
        }

        [[nodiscard]] std::size_t GetSampledNumber() const noexcept {
            return num_sampled_;
        }

        [[nodiscard]] std::size_t GetColumnMatchIndex() const noexcept {
            return column_match_index_;
        }

        [[nodiscard]] std::size_t GetParameter() const noexcept {
            return parameter_;
        }

        void IncrementParameter() noexcept {
            ++parameter_;
        }

        void CountComparedPair() noexcept {
            ++num_sampled_;
        }

        void CountSuccessfulComparison() noexcept {
            ++num_discovered_;
        }
    };

    static constexpr double kInitialEfficiencyThreshold = 0.01;
    static constexpr double kNewRoundThresholdMultiplier = 0.5;
    // In case sampling the best column match is still too inefficient.
    static constexpr double kBestThresholdMultiplier = 0.9;

    template <bool SampleShort>
    std::pair<std::size_t, Sampler<SampleShort>> CreateSampler(
            ColumnMatchSamplingInfo const& column_match_sampling_info);

    std::priority_queue<ColumnMatchSamplingInfo> CreateSamplingQueue();

    bool MultiThreaded() const noexcept {
        return pool_ != nullptr;
    }

    void InferFromRecommendationsParallel(Recommendations const& recommendations);
    void InferFromRecommendationsSeq(Recommendations const& recommendations);

    void InferFromRecommendations(Recommendations const& recommendations) {
        (this->*recommendations_inference_method_)(recommendations);
    }

    bool InferFromNew(PairComparisonResult const& pair_comparison_result);

    [[nodiscard]] PairComparisonResult CompareRecords(CompressedRecord const& left_record,
                                                      CompressedRecord const& right_record) const;

    bool InferFromComparison(PairComparisonResult&& comparison) {
        auto const& [it, not_seen_before] = processed_comparisons_.insert(std::move(comparison));
        if (not_seen_before) return InferFromNew(*it);
        return false;
    }

    void CountAndInfer(ColumnMatchSamplingInfo& column_match_sampling_info,
                       PairComparisonResult&& comparison) {
        column_match_sampling_info.CountComparedPair();
        if (InferFromComparison(std::move(comparison)))
            column_match_sampling_info.CountSuccessfulComparison();
    }

    bool ShortSamplingEnabled(model::Index column_match_index) const noexcept {
        if (!records_info_->OneTableGiven() || !sample_short_[column_match_index]) return false;
        auto const& [_, left_column_index, right_column_index] =
                (*column_matches_sim_info_)[column_match_index];
        return left_column_index == right_column_index;
    }

    template <bool IsShort>
    void DoSamplingRoundSeq(ColumnMatchSamplingInfo& column_match_sampling_info);
    template <bool IsShort>
    void DoSamplingRoundParallel(ColumnMatchSamplingInfo& column_match_sampling_info);

    // Short sampling: do nearest neighbor comparison similar to HyFD, parameter is window size,
    // sort based on CCV ID in the column match, then on the CCV ID to the left, then to the right.
    // Full sampling: compare RHS record at index parameter with every LHS record.
    void DoSamplingRound(ColumnMatchSamplingInfo& column_match_sampling_info) {
        if (ShortSamplingEnabled(column_match_sampling_info.GetColumnMatchIndex())) {
            (this->*short_sampling_method_)(column_match_sampling_info);
        } else {
            (this->*full_sampling_method_)(column_match_sampling_info);
        }
        column_match_sampling_info.IncrementParameter();
    }

    std::vector<Comparisons> CollectParallelComparisonResults(auto compare_at,
                                                              std::size_t index_limit);
    void ParallelCompareAndInfer(auto compare, std::size_t index_limit, auto comparison_action);

    void SampleAndRequeue(ColumnMatchSamplingInfo& column_match_sampling_info);
    bool SampleAndInfer();

    lattice::MdLattice* const lattice_;

    indexes::RecordsInfo const* const records_info_;
    std::vector<ColumnMatchInfo> const* const column_matches_sim_info_;
    std::vector<LhsCCVIdsInfo> const* const lhs_ccv_id_info_;
    std::size_t const column_match_number_ = column_matches_sim_info_->size();
    // For every column match that has a 1 in the bitset, its similarity measure is symmetrical and
    // equality of values means a 1.0 result.
    std::vector<bool> const sample_short_;
    std::unordered_set<PairComparisonResult> processed_comparisons_;
    util::WorkerThreadPool* const pool_;

    using SamplingMethod = void (RecordPairInferrer::*)(ColumnMatchSamplingInfo&);
    SamplingMethod short_sampling_method_ =
            MultiThreaded() ? &RecordPairInferrer::DoSamplingRoundParallel<true>
                            : &RecordPairInferrer::DoSamplingRoundSeq<true>;
    SamplingMethod full_sampling_method_ =
            MultiThreaded() ? &RecordPairInferrer::DoSamplingRoundParallel<false>
                            : &RecordPairInferrer::DoSamplingRoundSeq<false>;

    // For short sampling: cluster records come first, then same order as full sampling.
    // Cluster records are sorted such that identical values are grouped together.
    // For full sampling: records from similarity indexes.
    // If full sorting is enabled, records are ranked lexicographically, where words are sorted
    // CCV ID lists in comparisons with cluster records for the column match that is samples, with
    // neighboring column matches being tie breakers.
    // If sorting is disabled, the records with the highest CCV IDs of the sampled column match go
    // first, in whatever order they happen to land.
    std::vector<RankedRecordsColumnMatch> const ranked_records_;
    std::priority_queue<ColumnMatchSamplingInfo> sampling_queue_;

    // The first sampling round multiplies by kNewRoundThresholdMultiplier, divide to counteract.
    double efficiency_threshold_ = kInitialEfficiencyThreshold / kNewRoundThresholdMultiplier;

    using InferFromRecommendationsMethod = void (RecordPairInferrer::*)(Recommendations const&);
    InferFromRecommendationsMethod recommendations_inference_method_ =
            MultiThreaded() ? &RecordPairInferrer::InferFromRecommendationsParallel
                            : &RecordPairInferrer::InferFromRecommendationsSeq;

public:
    template <typename... Args>
    static std::pair<RecordPairInferrer, bool> Create(Args&&... args) {
        std::pair<RecordPairInferrer, bool> pair{
                std::piecewise_construct,
                std::forward_as_tuple(InternalConstructToken{}, std::forward<Args>(args)...),
                std::forward_as_tuple(false)};
        auto& [inferrer, out_of_pairs] = pair;
        if (inferrer.sampling_queue_.empty()) {
            out_of_pairs = true;
        } else {
            out_of_pairs = inferrer.InferFromRecordPairs({});
        }
        return pair;
    }

    RecordPairInferrer(InternalConstructToken, lattice::MdLattice* lattice,
                       indexes::RecordsInfo const* records_info,
                       std::vector<ColumnMatchInfo> const* column_matches_sim_info,
                       std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info,
                       std::vector<bool> sample_short, util::WorkerThreadPool* pool);

    bool InferFromRecordPairs(Recommendations const& recommendations);
};

}  // namespace algos::hymd
