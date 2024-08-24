#pragma once

#include <cassert>
#include <cstdint>
#include <list>
#include <queue>
#include <span>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

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
    using ValueRankedRecords = std::vector<RecordIdentifier>;
    using ColumnMatchRankedRecords = std::vector<ValueRankedRecords>;

    using SamplingBodyVariables =
            std::tuple<indexes::CompressedRecords const&, indexes::CompressedRecords const&,
                       std::vector<indexes::PliCluster> const&, ColumnMatchRankedRecords const&,
                       model::Index>;
    using SamplingVariables = std::pair<std::size_t, SamplingBodyVariables>;

    using InitializationBodyVariables =
            std::tuple<indexes::SimilarityIndex const&, std::vector<indexes::PliCluster> const&>;
    using InitializationVariables =
            std::tuple<std::size_t, ColumnMatchRankedRecords&, InitializationBodyVariables>;
    using SamplingOrderStats = std::vector<std::pair<ColumnClassifierValueId, unsigned>>;

    struct FullSamplingSortComparer {
        using StatMap = boost::unordered::unordered_flat_map<RecordIdentifier, SamplingOrderStats>;

        StatMap order_stats_prev;
        StatMap order_stats_next;

        bool operator()(RecordIdentifier rec1, RecordIdentifier rec2) const noexcept;
    };

    struct ShortSamplingClusterComparer {
        struct Info {
            model::Index const left_pli_index;
            model::Index const right_pli_index;
        };

        std::vector<CompressedRecord> const& records;
        Info prev_info;
        Info next_info;

        bool operator()(RecordIdentifier record_id1, RecordIdentifier record_id2) const noexcept;
    };

    // Using the sorting from HyFD. It can work okay here if equality implies highest CCV ID, which
    // is the case for all measures.
    ShortSamplingClusterComparer CreateShortSamplingClusterComparer(
            model::Index const column_match_index) const noexcept {
        auto const [prev_column_match_index, next_column_match_index] =
                GetPrevAndNext(column_match_index);
        std::vector<CompressedRecord> const& records =
                records_info_->GetLeftCompressor().GetRecords();
        std::vector<ColumnMatchInfo> const& cm_sim_info = *column_matches_sim_info_;

        ColumnMatchInfo const& prev_cm_info = cm_sim_info[prev_column_match_index];
        ColumnMatchInfo const& next_cm_info = cm_sim_info[next_column_match_index];
        ShortSamplingClusterComparer::Info prev_info{prev_cm_info.left_column_index,
                                                     prev_cm_info.right_column_index};
        ShortSamplingClusterComparer::Info next_info{next_cm_info.left_column_index,
                                                     next_cm_info.right_column_index};
        ;
        return {records, prev_info, next_info};
    }

    struct ShortSamplingNonClusterComparer {
        using ValueMatrixRowRef = indexes::SimilarityMatrixRow const&;

        struct Info {
            model::Index const pli_index;
            std::vector<ColumnClassifierValueId> const& rhs_lhs_map;
            indexes::SimilarityMatrix const& matrix;
        };

        bool LongBlock(ColumnClassifierValueId num) {
            static_assert(sizeof(ColumnClassifierValueId) == 2);
            return num >= 256;
        }

        std::span<RecordIdentifier> cluster_span;
        std::vector<CompressedRecord> const& records;
        Info prev_info;
        Info next_info;
        std::size_t const cluster_size = cluster_span.size();
        bool long_block =
                LongBlock(prev_info.rhs_lhs_map.back()) || LongBlock(next_info.rhs_lhs_map.back());

        bool RecLessThan(RecordIdentifier rec1, RecordIdentifier rec2) const noexcept;

        auto RecordComparer() {
            return [this](RecordIdentifier rec1, RecordIdentifier rec2) {
                return RecLessThan(rec1, rec2);
            };
        }

        std::uint16_t GetBracket(RecordIdentifier record_id, model::Index offset);

        auto BracketGetter() {
            return [this](RecordIdentifier record_id, model::Index offset) {
                return GetBracket(record_id, offset);
            };
        }

        auto SizeGetter() {
            std::size_t size = cluster_span.size() * 2;
            return [size](RecordIdentifier) { return size; };
        }
    };

    ShortSamplingNonClusterComparer CreateShortSamplingNonClusterComparer(
            model::Index const column_match_index,
            std::span<RecordIdentifier> cluster_span) const noexcept {
        auto const [prev_column_match_index, next_column_match_index] =
                GetPrevAndNext(column_match_index);
        std::vector<CompressedRecord> const& records =
                records_info_->GetLeftCompressor().GetRecords();
        std::vector<ColumnMatchInfo> const& cm_sim_info = *column_matches_sim_info_;
        std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info = *lhs_ccv_id_info_;

        ColumnMatchInfo const& prev_cm_info = cm_sim_info[prev_column_match_index];
        std::vector<ColumnClassifierValueId> const& prev_rhs_lhs_map =
                lhs_ccv_id_info[prev_column_match_index].rhs_to_lhs_map;
        indexes::SimilarityMatrix const& prev_matrix =
                prev_cm_info.similarity_info.similarity_matrix;
        model::Index prev_pli_index = prev_cm_info.left_column_index;
        assert(prev_pli_index == prev_cm_info.right_column_index);
        ShortSamplingNonClusterComparer::Info prev_info{prev_pli_index, prev_rhs_lhs_map,
                                                        prev_matrix};

        ColumnMatchInfo const& next_cm_info = cm_sim_info[next_column_match_index];
        std::vector<ColumnClassifierValueId> const& next_rhs_lhs_map =
                lhs_ccv_id_info[next_column_match_index].rhs_to_lhs_map;
        indexes::SimilarityMatrix const& next_matrix =
                next_cm_info.similarity_info.similarity_matrix;
        model::Index next_pli_index = next_cm_info.left_column_index;
        assert(next_pli_index == next_cm_info.right_column_index);
        ShortSamplingNonClusterComparer::Info next_info{next_pli_index, next_rhs_lhs_map,
                                                        next_matrix};

        return {cluster_span, records, prev_info, next_info};
    }

    class Efficiency {
    private:
        std::size_t column_match_index_;

        std::size_t parameter_;
        std::size_t num_discovered_ = 0;
        std::size_t num_comparisons_ = 0;

    public:
        Efficiency(std::size_t column_match_index, std::size_t parameter) noexcept
            : column_match_index_(column_match_index), parameter_(parameter) {}

        [[nodiscard]] double CalcEfficiency() const noexcept {
            DESBORDANTE_ASSUME(num_comparisons_ != 0);

            return static_cast<double>(num_discovered_) / static_cast<double>(num_comparisons_);
        }

        std::size_t GetComparisons() const noexcept {
            return num_comparisons_;
        }

        void IncrementParameter() noexcept {
            ++parameter_;
        }

        void AddComparison() noexcept {
            ++num_comparisons_;
        }

        void AddDiscovered() noexcept {
            ++num_discovered_;
        }

        [[nodiscard]] size_t GetColumnMatchIndex() const noexcept {
            return column_match_index_;
        }

        [[nodiscard]] unsigned GetParameter() const noexcept {
            return parameter_;
        }

        bool operator<(Efficiency const& other) const noexcept {
            return CalcEfficiency() < other.CalcEfficiency();
        }
    };

    lattice::MdLattice* lattice_;

    indexes::RecordsInfo const* records_info_;
    std::vector<ColumnMatchInfo> const* column_matches_sim_info_;
    std::vector<LhsCCVIdsInfo> const* lhs_ccv_id_info_;
    std::size_t left_size_ = records_info_->GetLeftCompressor().GetNumberOfRecords();
    std::size_t column_match_number_ = column_matches_sim_info_->size();
    // One table + all sim measures are symmetrical + equality means 1.0
    std::vector<bool> sample_short_;
    std::priority_queue<Efficiency> efficiency_queue_;
    // Sets for nearest-neighbor comparison for short sampling, RHS records for full sampling (LHS
    // records are left table's clusters' records)
    // Records are ranked lexicographically, where words are the numbers of CCV IDs in comparisons.
    std::vector<ColumnMatchRankedRecords> ranked_records_;

    std::unordered_set<PairComparisonResult> processed_comparisons_;

    static constexpr double kNormalThresholdDecrease = 0.5;
    // the first sampling round multiplies by this constant
    double efficiency_threshold_ = 0.01 / kNormalThresholdDecrease;

    util::WorkerThreadPool* pool_;

    RecordPairInferrer(lattice::MdLattice* lattice, indexes::RecordsInfo const* records_info,
                       std::vector<ColumnMatchInfo> const* column_matches_sim_info,
                       std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info,
                       std::vector<bool> sample_short, util::WorkerThreadPool* pool) noexcept
        : lattice_(lattice),
          records_info_(records_info),
          column_matches_sim_info_(column_matches_sim_info),
          lhs_ccv_id_info_(&lhs_ccv_id_info),
          sample_short_(std::move(sample_short)),
          pool_(pool) {}

    bool MultiThreaded() const noexcept {
        return pool_;
    }

    using SamplingMethod = void (RecordPairInferrer::*)(Efficiency&);
    SamplingMethod sampling_method_ =
            MultiThreaded() ? &RecordPairInferrer::SampleParallel : &RecordPairInferrer::SampleSeq;

    void ProcessRecommendationsParallel(Recommendations const& recommendations);
    void ProcessRecommendationsSeq(Recommendations const& recommendations);

    using ProcessRecommendationsMethod = void (RecordPairInferrer::*)(Recommendations const&);
    ProcessRecommendationsMethod recom_process_method_ =
            MultiThreaded() ? &RecordPairInferrer::ProcessRecommendationsParallel
                            : &RecordPairInferrer::ProcessRecommendationsSeq;

    bool ProcessPairComparison(PairComparisonResult const& pair_comparison_result);

    // Short sampling: do nearest neighbor comparison similar to HyFD, parameter is window size,
    // sort based on CCV ID in the column match, then on the CCV ID to the left, then to the right
    // Full sampling: compare RHS record at index parameter with every LHS record.

    [[nodiscard]] PairComparisonResult CompareRecords(CompressedRecord const& left_record,
                                                      CompressedRecord const& right_record) const;

    bool TryProcessComparison(PairComparisonResult&& comparison) {
        auto const& [it, not_seen_before] = processed_comparisons_.insert(std::move(comparison));
        if (not_seen_before) return ProcessPairComparison(*it);
        return false;
    }

    void AddComparison(Efficiency& efficiency, PairComparisonResult&& comparison) {
        efficiency.AddComparison();
        if (TryProcessComparison(std::move(comparison))) efficiency.AddDiscovered();
    }

    SamplingVariables GetSamplingVariables(Efficiency const& efficiency) {
        model::Index const column_match_index = efficiency.GetColumnMatchIndex();
        ColumnMatchInfo const& column_match_info = (*column_matches_sim_info_)[column_match_index];
        model::Index const left_pli_index = column_match_info.left_column_index;
        indexes::DictionaryCompressor const& compressor = records_info_->GetLeftCompressor();
        indexes::CompressedRecords const& left_records = compressor.GetRecords();
        indexes::CompressedRecords const& right_records =
                records_info_->GetRightCompressor().GetRecords();
        indexes::KeyedPositionListIndex const& left_pli = compressor.GetPli(left_pli_index);
        std::vector<indexes::PliCluster> const& clusters = left_pli.GetClusters();
        std::size_t clusters_number = clusters.size();
        ColumnMatchRankedRecords const& ranked_column_match_records =
                ranked_records_[column_match_index];
        model::Index const parameter = efficiency.GetParameter();
        return {clusters_number,
                {left_records, right_records, clusters, ranked_column_match_records, parameter}};
    }

    InitializationVariables GetInitializationVariables(Efficiency const& efficiency) {
        model::Index const column_match_index = efficiency.GetColumnMatchIndex();
        ColumnMatchRankedRecords& column_match_ranked_records = ranked_records_[column_match_index];
        indexes::DictionaryCompressor const& left_compressor = records_info_->GetLeftCompressor();
        ColumnMatchInfo const& column_match_info = (*column_matches_sim_info_)[column_match_index];
        model::Index const left_pli_index = column_match_info.left_column_index;
        std::vector<indexes::PliCluster> const& left_clusters =
                left_compressor.GetPli(left_pli_index).GetClusters();
        indexes::SimilarityIndex const& upper_set_index =
                column_match_info.similarity_info.similarity_index;
        std::size_t const total_left_values = left_clusters.size();
        return {total_left_values, column_match_ranked_records, {upper_set_index, left_clusters}};
    }

    bool ShortSamplingEnabled(model::Index column_match_index) {
        if (!records_info_->OneTableGiven() || !sample_short_[column_match_index]) return false;
        auto const& [_, left_column_index, right_column_index] =
                (*column_matches_sim_info_)[column_match_index];
        return left_column_index == right_column_index;
    }

    std::pair<model::Index, model::Index> GetPrevAndNext(
            model::Index column_match_index) const noexcept {
        model::Index const last_cm = column_match_number_ - 1;
        model::Index const prev_column_match_index =
                column_match_index == 0 ? last_cm : column_match_index - 1;
        model::Index const next_column_match_index =
                column_match_index == last_cm ? 0 : column_match_index + 1;
        return {prev_column_match_index, next_column_match_index};
    }

    FullSamplingSortComparer CreateComparer(indexes::PliCluster const& cluster,
                                            ValueRankedRecords const& right_records,
                                            model::Index column_match_index) const {
        auto const [prev_column_match_index, next_column_match_index] =
                GetPrevAndNext(column_match_index);

        std::size_t const record_number = right_records.size();
        FullSamplingSortComparer::StatMap order_stats_prev(record_number);
        for (RecordIdentifier record_id : right_records) {
            order_stats_prev[record_id] = CreateStats(record_id, prev_column_match_index, cluster);
        }
        FullSamplingSortComparer::StatMap order_stats_next(record_number);
        for (RecordIdentifier record_id : right_records) {
            order_stats_next[record_id] = CreateStats(record_id, next_column_match_index, cluster);
        }
        return {std::move(order_stats_prev), std::move(order_stats_next)};
    }

    template <bool IsShort>
    struct SampleLoopBody;
    template <bool IsShort, typename ObtainValueRecords>
    struct InitializeLoopBody;

    template <bool IsShort>
    void SampleSeqImpl(Efficiency& efficiency);
    template <bool IsShort>
    void SampleParallelImpl(Efficiency& efficiency);
    template <bool IsShort>
    bool InitializeSeqImpl(Efficiency& efficiency);
    template <bool IsShort>
    bool InitializeParallelImpl(Efficiency& efficiency);

    void SampleSeq(Efficiency& efficiency);
    void SampleParallel(Efficiency& efficiency);

    void ParallelCompareAndProcess(auto compare, std::size_t index_limit, auto comparison_action);

    SamplingOrderStats CreateStats(RecordIdentifier right_record_id,
                                   model::Index column_match_index,
                                   indexes::PliCluster const& cluster) const;
    bool InitializeShortSeq(Efficiency& efficiency);
    bool InitializeFullSeq(Efficiency& efficiency);
    bool InitializeShortParallel(Efficiency& efficiency);
    bool InitializeFullParallel(Efficiency& efficiency);

    bool Initialize();
    static std::strong_ordering CompareRecStats(SamplingOrderStats const& rec1_stats,
                                                SamplingOrderStats const& rec2_stats) noexcept;

public:
    template <typename... Args>
    static std::pair<RecordPairInferrer, bool> Create(Args&&... args) {
        auto inferrer = RecordPairInferrer{std::forward<Args>(args)...};
        bool done = inferrer.Initialize();
        if (done) return {std::move(inferrer), true};
        return {std::move(inferrer), inferrer.InferFromRecordPairs({})};
    }

    bool InferFromRecordPairs(Recommendations const& recommendations);
};

}  // namespace algos::hymd
