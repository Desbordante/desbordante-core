#pragma once

#include <cstddef>
#include <queue>
#include <unordered_set>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"
#include "algorithms/mde/hymde/cover_calculation/pair_comparison_result.h"
#include "algorithms/mde/hymde/cover_calculation/recommendation.h"
#include "algorithms/mde/hymde/record_identifier.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "util/desbordante_assume.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::cover_calculation {
class RecordPairInferrer {
private:
    using Comparisons = std::vector<PairComparisonResult>;

    using RankedRecordsValue = std::vector<RecordIdentifier>;
    using RankedRecordsRecordMatch = std::vector<RankedRecordsValue>;

    class RecordRanker;

    template <bool SampleShort>
    class Sampler;

    class InternalConstructToken {
        friend RecordPairInferrer;
        InternalConstructToken() = default;
    };

    // "RHS record" in the comments refers to a record from the right table present in a similarity
    // index.
    class RecordMatchSamplingInfo {
    private:
        model::Index record_match_index_;

        // Window size for short sampling on cluster or RHS record index otherwise.
        std::size_t parameter_;

        // Number of pairs where any existing dependency assumed to hold was violated.
        // Possible alternative approach similar to HyFD: number of pairs with a comparison result
        // not encountered before.
        std::size_t num_discovered_ = 0;
        // Number of pairs that were sampled (i.e. selected for comparison).
        std::size_t num_sampled_ = 0;

    public:
        RecordMatchSamplingInfo(std::size_t record_match_index, std::size_t parameter) noexcept
            : record_match_index_(record_match_index), parameter_(parameter) {}

        // Only used in operator<.
        [[nodiscard]] double CalcEfficiency() const noexcept {
            // Record matches where no record pairs are available for comparison are filtered during
            // RecordPairInferrer construction, after which this number can only increase.
            DESBORDANTE_ASSUME(num_sampled_ != 0);

            return static_cast<double>(num_discovered_) / static_cast<double>(num_sampled_);
        }

        // Used by the sampling queue to compare record matches.
        [[nodiscard]] bool operator<(RecordMatchSamplingInfo const& other) const noexcept {
            return CalcEfficiency() < other.CalcEfficiency();
        }

        [[nodiscard]] std::size_t GetSampledNumber() const noexcept {
            return num_sampled_;
        }

        [[nodiscard]] model::Index GetRecordMatchIndex() const noexcept {
            return record_match_index_;
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
    // In case sampling the best record match is still too inefficient.
    static constexpr double kBestThresholdMultiplier = 0.9;

    template <bool SampleShort>
    std::pair<std::size_t, Sampler<SampleShort>> CreateSampler(
            RecordMatchSamplingInfo const& record_match_sampling_info);

    std::priority_queue<RecordMatchSamplingInfo> CreateSamplingQueue();

    bool MultiThreaded() const noexcept {
        return pool_ != nullptr;
    }

    void InferFromRecommendationsParallel(Recommendations const& recommendations);
    void InferFromRecommendationsSeq(Recommendations const& recommendations);

    void InferFromRecommendations(Recommendations const& recommendations) {
        (this->*recommendations_inference_method_)(recommendations);
    }

    bool InferFromNew(PairComparisonResult const& pair_comparison_result);

    [[nodiscard]] PairComparisonResult CompareRecords(
            record_match_indexes::PartitionIndex::Clusters const& left_record,
            record_match_indexes::PartitionIndex::Clusters const& right_record) const;

    bool InferFromComparison(PairComparisonResult&& comparison) {
        auto const& [it, not_seen_before] = processed_comparisons_.insert(std::move(comparison));
        if (not_seen_before) return InferFromNew(*it);
        return false;
    }

    void CountAndInfer(RecordMatchSamplingInfo& record_match_sampling_info,
                       PairComparisonResult&& comparison) {
        record_match_sampling_info.CountComparedPair();
        if (InferFromComparison(std::move(comparison)))
            record_match_sampling_info.CountSuccessfulComparison();
    }

    bool ShortSamplingEnabled(model::Index record_match_index) const noexcept {
        return assertions_[record_match_index].assume_overlap_lpli_cluster_max_;
    }

    template <bool IsShort>
    void DoSamplingRoundSeq(RecordMatchSamplingInfo& record_match_sampling_info);
    template <bool IsShort>
    void DoSamplingRoundParallel(RecordMatchSamplingInfo& record_match_sampling_info);

    // Short sampling: do nearest neighbor comparison similar to HyFD, parameter is window size,
    // sort based on RCV ID in the record match, then on the RCV ID to the left, then to the right.
    // Full sampling: compare RHS record at index parameter with every LHS record.
    void DoSamplingRound(RecordMatchSamplingInfo& record_match_sampling_info) {
        if (ShortSamplingEnabled(record_match_sampling_info.GetRecordMatchIndex())) {
            (this->*short_sampling_method_)(record_match_sampling_info);
        } else {
            (this->*full_sampling_method_)(record_match_sampling_info);
        }
        record_match_sampling_info.IncrementParameter();
    }

    std::vector<Comparisons> CollectParallelComparisonResults(auto compare_at,
                                                              std::size_t index_limit);
    void ParallelCompareAndInfer(auto compare, std::size_t index_limit, auto comparison_action);

    void SampleAndRequeue(RecordMatchSamplingInfo& record_match_sampling_info);
    bool SampleAndInfer();

    lattice::MdeLattice* const lattice_;

    record_match_indexes::DataPartitionIndex const* const records_info_;
    std::vector<record_match_indexes::Indexes> const* const record_match_indexes_;
    std::vector<record_match_indexes::RcvIdLRMap> const* const rcv_id_lr_maps_;
    std::size_t const record_match_number_ = record_match_indexes_->size();
    std::vector<record_match_indexes::ComponentStructureAssertions> const assertions_;
    std::unordered_set<PairComparisonResult> processed_comparisons_;
    util::WorkerThreadPool* const pool_;

    using SamplingMethod = void (RecordPairInferrer::*)(RecordMatchSamplingInfo&);
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
    // RCV ID lists in comparisons with cluster records for the record match that is samples, with
    // neighboring record matches being tie breakers.
    // If sorting is disabled, the records with the highest RCV IDs of the sampled record match go
    // first, in whatever order they happen to land.
    std::vector<RankedRecordsRecordMatch> const ranked_records_;
    std::priority_queue<RecordMatchSamplingInfo> sampling_queue_;

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

    RecordPairInferrer(InternalConstructToken, lattice::MdeLattice* lattice,
                       record_match_indexes::DataPartitionIndex const* records_info,
                       std::vector<record_match_indexes::Indexes> const* record_match_indexes,
                       std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
                       std::vector<record_match_indexes::ComponentStructureAssertions> assertions,
                       util::WorkerThreadPool* pool);

    bool InferFromRecordPairs(Recommendations const& recommendations);
};
}  // namespace algos::hymde::cover_calculation
