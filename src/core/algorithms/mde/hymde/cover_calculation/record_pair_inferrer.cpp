#include "algorithms/mde/hymde/cover_calculation/record_pair_inferrer.h"

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/value_matrix.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "model/index.h"

namespace {
algos::hymde::RecordClassifierValueId GetRCVId(
        algos::hymde::record_match_indexes::ValueMatrixRow const& row,
        algos::hymde::PartitionValueId rhs_value) {
    auto rcv_id_it = row.find(rhs_value);
    return rcv_id_it == row.end() ? algos::hymde::kLowestRCValueId : rcv_id_it->second;
}
}  // namespace

namespace algos::hymde::cover_calculation {
using PartitionValueIdMap = record_match_indexes::PartitionIndex::PartitionValueIdMap;
using PValueIdMapPtr = record_match_indexes::PartitionIndex::PartitionValueIdMap const*;

// TODO: put record pairs that have the highest LHS RCV IDs first, but do not sort based on that
// (takes too long, and realistically not all pairs are going to be sampled).
class RecordPairInferrer::RecordRanker {
    using RankingBodyVariables =
            std::tuple<record_match_indexes::UpperSetIndex const&,
                       record_match_indexes::PartitionIndex::PositionListIndex const&>;
    using RankingVariables = std::tuple<std::size_t, RankingBodyVariables>;

    RecordPairInferrer const& inferrer_;

    struct ShortSamplingClusterComparer {
        struct Info {
            model::Index const prev_record_match_index;
            model::Index const next_record_match_index;
        };

        std::vector<PartitionValueIdMap> const& records;
        Info prev_info;
        Info next_info;

        bool operator()(RecordIdentifier record_id1, RecordIdentifier record_id2) const noexcept;
    };

    template <bool IsShort, typename ObtainValueRecords>
    struct RankRecordsLoopBody;

    std::pair<model::Index, model::Index> GetPrevAndNext(
            model::Index const record_match_index) const noexcept {
        // Prevented in the main part of the algorithm.
        DESBORDANTE_ASSUME(inferrer_.record_match_number_ != 0);
        model::Index const last_cm = inferrer_.record_match_number_ - 1;
        model::Index const prev_record_match_index =
                record_match_index == 0 ? last_cm : record_match_index - 1;
        model::Index const next_record_match_index =
                record_match_index == last_cm ? 0 : record_match_index + 1;
        return {prev_record_match_index, next_record_match_index};
    }

    ShortSamplingClusterComparer CreateShortSamplingClusterComparer(
            model::Index const record_match_index) const noexcept {
        auto const [prev_record_match_index, next_record_match_index] =
                GetPrevAndNext(record_match_index);
        record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& records =
                inferrer_.records_info_->GetLeft().GetPartitionValueIdMaps();

        ShortSamplingClusterComparer::Info prev_info{prev_record_match_index,
                                                     next_record_match_index};
        ShortSamplingClusterComparer::Info next_info{prev_record_match_index,
                                                     next_record_match_index};
        return {records, prev_info, next_info};
    }

    RankingVariables GetRankingVariables(model::Index const record_match_index) {
        auto const& [value_matrix, upper_set_index] =
                (*inferrer_.record_match_indexes_)[record_match_index];

        record_match_indexes::PartitionIndex::PositionListIndex const& left_clusters =
                inferrer_.records_info_->GetLeft().GetPli(record_match_index);

        std::size_t const total_left_values = left_clusters.size();
        return {total_left_values, {upper_set_index, left_clusters}};
    }

    template <bool IsShort>
    void RankRecordsSeq(model::Index const record_match_index,
                        RankedRecordsRecordMatch& record_match_ranked_records);
    template <bool IsShort>
    void RankRecordsParallel(model::Index const record_match_index,
                             RankedRecordsRecordMatch& ranked_records_record_match);

public:
    RecordRanker(RecordPairInferrer const& inferrer) : inferrer_(inferrer) {}

    std::vector<RankedRecordsRecordMatch> RankRecords();
};

// Using the sorting from HyFD. It can work okay here if equality implies highest RCV ID, which is
// the case for all built-in comparison functions.
bool RecordPairInferrer::RecordRanker::ShortSamplingClusterComparer::operator()(
        RecordIdentifier record_id1, RecordIdentifier record_id2) const noexcept {
    PartitionValueIdMap const& record1 = records[record_id1];
    PartitionValueIdMap const& record2 = records[record_id2];
    for (model::Index record_match_index :
         {prev_info.prev_record_match_index, prev_info.next_record_match_index,
          next_info.prev_record_match_index, next_info.next_record_match_index}) {
        auto value_id_order = record1[record_match_index] <=> record2[record_match_index];
        if (value_id_order != std::strong_ordering::equal)
            return value_id_order == std::strong_ordering::less;
    }
    return false;
}

//  Short sampling uses a sliding window for sampling cluster records.
//  Short sort, ideally: put the records in such an order that the sorted list of RCV IDs from the
// previous record match is the greatest lexicographically for the first sampling round (window_size
// = 1), if equal, same for the subsequent rounds until not equal. If all equal, sort in the same
// way for the right record match. Apply the above to every span of the same RCV ID in the current
// record match. This is the desired order, but finding it is NP-hard.
//  Instead, this adapts HyFD's approach. The cluster's records are stored first, sorted in a
// similar way to HyFD, followed by other records, sorted by their RCV ID in record_match_index.
//  Full sorting is not implemented.
template <typename ObtainValueRecords>
struct RecordPairInferrer::RecordRanker::RankRecordsLoopBody<true, ObtainValueRecords> {
    RecordRanker const& ranker;
    RankingBodyVariables const variables;
    ObtainValueRecords obtain_value_records;
    model::Index const record_match_index;

    // Sorting the cluster in a way that the first round of sampling yields the most of the highest
    // RCV IDs is an NP-hard problem: an algorithm obtaining such a permutation can be used for
    // solving the Hamiltonian path problem, which is NP-complete.
    void OrderCluster(std::vector<RecordIdentifier>& cluster_records) const noexcept {
        ShortSamplingClusterComparer comparer =
                ranker.CreateShortSamplingClusterComparer(record_match_index);
        std::ranges::sort(cluster_records, comparer);
    }

    void operator()(PartitionValueId left_value_id) {
        auto const [upper_set_index, left_clusters] = variables;
        auto const& [right_records, end_id_map] = upper_set_index[left_value_id].GetFlat();

        RankedRecordsValue& value_ranked_records = obtain_value_records(left_value_id);
        value_ranked_records.reserve(right_records.size());

        record_match_indexes::PartitionIndex::PliCluster const& cluster =
                left_clusters[left_value_id];
        assert(std::ranges::all_of(cluster, [&](RecordIdentifier left_record_id) {
            return std::ranges::find(right_records, left_record_id) != right_records.end();
        }));
        value_ranked_records.insert(value_ranked_records.end(), cluster.begin(), cluster.end());

        OrderCluster(value_ranked_records);

        std::unordered_set<RecordIdentifier> cluster_set(cluster.begin(), cluster.end());
        for (RecordIdentifier record_id : right_records) {
            if (!cluster_set.contains(record_id)) value_ranked_records.push_back(record_id);
        }
        // Full sampling sorting goes here.
    }
};

template <typename ObtainValueRecords>
struct RecordPairInferrer::RecordRanker::RankRecordsLoopBody<false, ObtainValueRecords> {
    RecordRanker const& ranker;
    RankingBodyVariables const variables;
    ObtainValueRecords obtain_value_records;
    model::Index const record_match_index;

    void operator()(PartitionValueId left_value_id) {
        auto const [upper_set_index, left_clusters] = variables;
        auto const& [right_records, end_id_map] = upper_set_index[left_value_id].GetFlat();

        RankedRecordsValue& value_ranked_records = obtain_value_records(left_value_id);
        value_ranked_records.reserve(right_records.size());

        value_ranked_records.insert(value_ranked_records.end(), right_records.begin(),
                                    right_records.end());
    }
};

template <bool IsShort>
void RecordPairInferrer::RecordRanker::RankRecordsParallel(
        model::Index const record_match_index,
        RankedRecordsRecordMatch& record_match_ranked_records) {
    auto const [total_left_values, variables] = GetRankingVariables(record_match_index);
    // The table is not empty, so there are values.
    DESBORDANTE_ASSUME(total_left_values != 0);

    record_match_ranked_records.assign(total_left_values, {});
    auto obtain_at_index = [&](PartitionValueId value_id) -> decltype(auto) {
        return record_match_ranked_records[value_id];
    };
    auto loop_body = RankRecordsLoopBody<IsShort, decltype(obtain_at_index)>{
            *this, variables, obtain_at_index, record_match_index};
    auto rank_for = [&](PartitionValueId left_value_id) { loop_body(left_value_id); };
    inferrer_.pool_->ExecIndex(rank_for, total_left_values);
}

template <bool IsShort>
void RecordPairInferrer::RecordRanker::RankRecordsSeq(
        model::Index const record_match_index,
        RankedRecordsRecordMatch& ranked_records_record_match) {
    auto const [total_left_values, variables] = GetRankingVariables(record_match_index);
    // The table is not empty, so there are values.
    DESBORDANTE_ASSUME(total_left_values != 0);

    auto obtain_new = [&](model::Index) -> decltype(auto) {
        return ranked_records_record_match.emplace_back();
    };
    auto loop_body = RankRecordsLoopBody<IsShort, decltype(obtain_new)>{
            *this, variables, obtain_new, record_match_index};
    for (PartitionValueId left_value_id : utility::IndexRange(total_left_values)) {
        loop_body(left_value_id);
    }
}

auto RecordPairInferrer::RecordRanker::RankRecords() -> std::vector<RankedRecordsRecordMatch> {
    std::vector<RankedRecordsRecordMatch> ranked_records =
            util::GetPreallocatedVector<RankedRecordsRecordMatch>(inferrer_.record_match_number_);

    using InitMethod = void (RecordRanker::*)(model::Index const, RankedRecordsRecordMatch&);
    InitMethod short_method = &RecordRanker::RankRecordsSeq<true>;
    InitMethod full_method = &RecordRanker::RankRecordsSeq<false>;
    if (inferrer_.MultiThreaded()) {
        short_method = &RecordRanker::RankRecordsParallel<true>;
        full_method = &RecordRanker::RankRecordsParallel<false>;
    }

    for (model::Index const record_match_index :
         utility::IndexRange(inferrer_.record_match_number_)) {
        RankedRecordsRecordMatch& ranked_records_record_match = ranked_records.emplace_back();
        // See CreateSamplingQueue comments.
        if ((*inferrer_.rcv_id_lr_maps_)[record_match_index].lhs_to_rhs_map.size() == 1) continue;

        InitMethod fill_records =
                inferrer_.ShortSamplingEnabled(record_match_index) ? short_method : full_method;
        (this->*fill_records)(record_match_index, ranked_records_record_match);
    }

    return ranked_records;
}

auto RecordPairInferrer::CreateSamplingQueue() -> std::priority_queue<RecordMatchSamplingInfo> {
    std::vector<RecordMatchSamplingInfo> sampling_info =
            util::GetPreallocatedVector<RecordMatchSamplingInfo>(record_match_number_);
    for (model::Index record_match_index : utility::IndexRange(record_match_number_)) {
        auto const& [lhs_rhs_map, rhs_lhs_map] = (*rcv_id_lr_maps_)[record_match_index];
        // TODO: enforce this invariant with a class.
        DESBORDANTE_ASSUME(!lhs_rhs_map.empty());
        // No records are matched.
        if (lhs_rhs_map.size() == 1) {
            // Trivial record matches have been excluded.
            DESBORDANTE_ASSUME(rhs_lhs_map.size() > 1);
            // TODO: sort based on record matches where the number of LHS values is greater
            // than 1, discard pairs with the highest RCV ID.
            LOG(WARNING) << "Sampling for record match " << record_match_index
                         << " not implemented.";
            continue;
        }

        // NOTE: If all record matches have equivalent partitioning functions and have a comparison
        // function that is symmetrical and outputs 1.0 for equal values, then comparing a record to
        // itself is useless because it does not refine the initial assumption, the comparison will
        // just yield maximum values for all record matches. Thus, the initial sampling will also be
        // completely useless. Initial parameter being 1 is meant to mitigate that. However, in
        // doing so, if it happens that no other record match sampling process compares a record to
        // itself, then the record pair with both elements being the same record will be left out,
        // making the sampling process technically incomplete. No big deal: lattice traversal will
        // still make the result correct. And a pair like that is likely to end up among
        // recommendations too.
        std::size_t const initial_parameter = ShortSamplingEnabled(record_match_index) ? 1 : 0;
        RecordMatchSamplingInfo& record_match_sampling_info =
                sampling_info.emplace_back(record_match_index, initial_parameter);
        DoSamplingRound(record_match_sampling_info);
        // No pairs have been sampled, and no pairs are going to be sampled if the parameter is
        // increased either.
        if (record_match_sampling_info.GetSampledNumber() == 0) sampling_info.pop_back();
    }
    return std::priority_queue<RecordMatchSamplingInfo>{decltype(sampling_queue_)::value_compare{},
                                                        std::move(sampling_info)};
}

RecordPairInferrer::RecordPairInferrer(
        InternalConstructToken, lattice::MdeLattice* lattice,
        record_match_indexes::DataPartitionIndex const* records_info,
        std::vector<record_match_indexes::Indexes> const* record_match_indexes,
        std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
        std::vector<record_match_indexes::ComponentStructureAssertions> assertions,
        util::WorkerThreadPool* pool)
    : lattice_(lattice),
      records_info_(records_info),
      record_match_indexes_(record_match_indexes),
      rcv_id_lr_maps_(&rcv_id_lr_maps),
      assertions_(std::move(assertions)),
      pool_(pool),
      ranked_records_(RecordRanker{*this}.RankRecords()),
      sampling_queue_(CreateSamplingQueue()) {}

PairComparisonResult RecordPairInferrer::CompareRecords(
        PartitionValueIdMap const& left_record, PartitionValueIdMap const& right_record) const {
    std::vector<RecordClassifierValueId> rhss =
            util::GetPreallocatedVector<RecordClassifierValueId>(record_match_number_);
    for (model::Index record_match_index : utility::IndexRange(record_match_number_)) {
        auto const& [value_matrix, upper_set_index] = (*record_match_indexes_)[record_match_index];
        record_match_indexes::ValueMatrixRow const& row =
                value_matrix[left_record[record_match_index]];
        rhss.push_back(GetRCVId(row, right_record[record_match_index]));
    }
    return {std::move(rhss), *rcv_id_lr_maps_};
}

bool RecordPairInferrer::InferFromNew(PairComparisonResult const& pair_comparison_result) {
    using MdRefiner = lattice::MdeLattice::PairUpdater;
    std::vector<MdRefiner> refiners = lattice_->CollectRefinersForViolated(pair_comparison_result);
    std::ranges::for_each(refiners, std::mem_fn(&MdRefiner::Refine));
    return !refiners.empty();
}

// TODO: use comparison function symmetry.
template <bool ShortSampling>
class RecordPairInferrer::Sampler {
    record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& left_records_;
    record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& right_records_;
    record_match_indexes::PartitionIndex::PositionListIndex const& clusters_;
    RankedRecordsRecordMatch const& record_match_ranked_records_;
    model::Index const parameter_;

    void SampleWholeCluster(record_match_indexes::PartitionIndex::PliCluster const& cluster,
                            RankedRecordsValue const& ranked_records_value,
                            auto&& record_pair_action) const {
        std::size_t const record_rank = parameter_;

        PartitionValueIdMap const& right_record = right_records_[ranked_records_value[record_rank]];
        for (RecordIdentifier left_record_id : cluster) {
            PartitionValueIdMap const& left_record = left_records_[left_record_id];
            record_pair_action(left_record, right_record);
        }
    }

    void SampleSlidingWindow(std::size_t const cluster_size,
                             RankedRecordsValue const& ranked_records_value,
                             auto&& record_pair_action) const {
        std::size_t const window_size = parameter_;

        // Short sampling relies on equal values being similar.
        DESBORDANTE_ASSUME(ranked_records_value.size() >= cluster_size);
        auto const pivot = ranked_records_value.begin();

        // Other records, if compared with the cluster's records, will not always yield a lot of the
        // same similarity sets, like they would if they were from the cluster, so the argument to
        // treat the cluster's records specially does not apply to them. If we did use sliding
        // window sampling for other records, the expression for the end iterator would need to be a
        // little more complicated.
        DESBORDANTE_ASSUME(window_size < cluster_size);
        auto const partner = pivot + window_size;
        auto const end = pivot + cluster_size;

        // Sliding window logic for MDE: table partition elements are exactly the same and values
        // from the same partition element yield the highest possible comparison result when
        // compared with each other.
        for (auto const [left_record_id, right_record_id] : utility::Zip(end, pivot, partner)) {
            PartitionValueIdMap const& right_record = right_records_[right_record_id];
            PartitionValueIdMap const& left_record = left_records_[left_record_id];
            record_pair_action(left_record, right_record);
        }
    }

    void SampleShort(record_match_indexes::PartitionIndex::PliCluster const& cluster,
                     RankedRecordsValue const& ranked_records_value,
                     auto&& record_pair_action) const {
        std::size_t const cluster_size = cluster.size();
        if (parameter_ < cluster_size) {
            SampleSlidingWindow(cluster_size, ranked_records_value, record_pair_action);
        } else {
            SampleWholeCluster(cluster, ranked_records_value, record_pair_action);
        }
    }

public:
    Sampler(record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& left_records,
            record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& right_records,
            record_match_indexes::PartitionIndex::PositionListIndex const& clusters,
            RankedRecordsRecordMatch const& ranked_records_record_match, model::Index parameter)
        : left_records_(left_records),
          right_records_(right_records),
          clusters_(clusters),
          record_match_ranked_records_(ranked_records_record_match),
          parameter_(parameter) {}

    void Sample(PartitionValueId value_id, auto&& record_pair_action) const {
        RankedRecordsValue const& ranked_records_value = record_match_ranked_records_[value_id];
        std::size_t const similar_record_number = ranked_records_value.size();
        if (parameter_ >= similar_record_number) return;

        if constexpr (ShortSampling) {
            SampleShort(clusters_[value_id], ranked_records_value, record_pair_action);
        } else {
            SampleWholeCluster(clusters_[value_id], ranked_records_value, record_pair_action);
        }
    }
};

template <bool SampleShort>
auto RecordPairInferrer::CreateSampler(RecordMatchSamplingInfo const& record_match_sampling_info)
        -> std::pair<std::size_t, Sampler<SampleShort>> {
    record_match_indexes::PartitionIndex const& left_compressor = records_info_->GetLeft();
    record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& left_records =
            left_compressor.GetPartitionValueIdMaps();

    record_match_indexes::PartitionIndex::TablePartitionValueIdMaps const& right_records =
            records_info_->GetRight().GetPartitionValueIdMaps();

    model::Index const record_match_index = record_match_sampling_info.GetRecordMatchIndex();

    record_match_indexes::PartitionIndex::PositionListIndex const& clusters =
            left_compressor.GetPli(record_match_index);

    RankedRecordsRecordMatch const& ranked_record_match_records =
            ranked_records_[record_match_index];

    model::Index const parameter = record_match_sampling_info.GetParameter();

    std::size_t const left_values_number = clusters.size();

    return {left_values_number,
            {left_records, right_records, clusters, ranked_record_match_records, parameter}};
}

auto RecordPairInferrer::CollectParallelComparisonResults(auto compare_at, std::size_t index_limit)
        -> std::vector<Comparisons> {
    std::vector<Comparisons> all_thread_comparisons(pool_->ThreadNum());
    std::atomic<Comparisons*> current_thread_comparisons = &all_thread_comparisons.front();
    auto get_thread_comparisons = [&current_thread_comparisons]() {
        return current_thread_comparisons.fetch_add(1, std::memory_order::acquire);
    };

    pool_->ExecIndexWithResource(compare_at, get_thread_comparisons, index_limit);
    return all_thread_comparisons;
}

void RecordPairInferrer::ParallelCompareAndInfer(auto compare_at, std::size_t index_limit,
                                                 auto infer_from) {
    std::vector<Comparisons> collected = CollectParallelComparisonResults(compare_at, index_limit);

    for (Comparisons& thread_comparisons : collected) {
        for (PairComparisonResult& comparison : thread_comparisons) {
            infer_from(std::move(comparison));
        }
    }
}

template <bool IsShort>
void RecordPairInferrer::DoSamplingRoundParallel(
        RecordMatchSamplingInfo& record_match_sampling_info) {
    auto const [left_values_number, sampler] = CreateSampler<IsShort>(record_match_sampling_info);

    auto compare_and_store_all_for_value = [this, &sampler](PartitionValueId left_value_id,
                                                            Comparisons* thread_comparisons) {
        auto compare_and_store = [this, thread_comparisons](
                                         PartitionValueIdMap const& left_record,
                                         PartitionValueIdMap const& right_record) {
            thread_comparisons->push_back(CompareRecords(left_record, right_record));
        };
        sampler.Sample(left_value_id, compare_and_store);
    };

    auto infer_from_compared = [&](PairComparisonResult&& comparison) {
        CountAndInfer(record_match_sampling_info, std::move(comparison));
    };
    ParallelCompareAndInfer(compare_and_store_all_for_value, left_values_number,
                            infer_from_compared);
}

template <bool IsShort>
void RecordPairInferrer::DoSamplingRoundSeq(RecordMatchSamplingInfo& record_match_sampling_info) {
    auto const [left_values_number, sampler] = CreateSampler<IsShort>(record_match_sampling_info);

    auto infer_immediately = [&](PartitionValueIdMap const& left_record,
                                 PartitionValueIdMap const& right_record) {
        CountAndInfer(record_match_sampling_info, CompareRecords(left_record, right_record));
    };

    for (PartitionValueId left_value_id : utility::IndexRange(left_values_number)) {
        sampler.Sample(left_value_id, infer_immediately);
    }
}

void RecordPairInferrer::SampleAndRequeue(RecordMatchSamplingInfo& record_match_sampling_info) {
    std::size_t const sampled_previously = record_match_sampling_info.GetSampledNumber();
    DoSamplingRound(record_match_sampling_info);
    std::size_t const sampled_after_round = record_match_sampling_info.GetSampledNumber();
    bool const out_of_pairs = sampled_previously == sampled_after_round;
    if (out_of_pairs) return;

    sampling_queue_.push(record_match_sampling_info);
}

bool RecordPairInferrer::SampleAndInfer() {
    // Method should not be called if the queue is empty, signaled by the return value of Create and
    // InferFromRecordPairs.
    DESBORDANTE_ASSUME(!sampling_queue_.empty());
    RecordMatchSamplingInfo best = sampling_queue_.top();
    sampling_queue_.pop();

    efficiency_threshold_ = std::min(efficiency_threshold_ * kNewRoundThresholdMultiplier,
                                     best.CalcEfficiency() * kBestThresholdMultiplier);

    while (true) {
        SampleAndRequeue(best);
        if (sampling_queue_.empty()) break;

        best = sampling_queue_.top();
        if (best.CalcEfficiency() < efficiency_threshold_) return false;
        sampling_queue_.pop();
    };
    return true;
}

void RecordPairInferrer::ForEachRecommendation(BatchValidator::Result const& result,
                                               auto&& action) {
    for (auto const& [left_record_pvid_map_ptrs, right_record_pvid_map_ptr] :
         result.lhs_grouped_recommendations) {
        for (PValueIdMapPtr left_record_pvid_map_ptr : left_record_pvid_map_ptrs) {
            action(CompareRecords(*left_record_pvid_map_ptr, *right_record_pvid_map_ptr));
        }
    }
}

void RecordPairInferrer::InferFromRecommendationsParallel(
        std::vector<BatchValidator::Result> const& results) {
    auto compare_records_and_store = [this, &results](model::Index index,
                                                      Comparisons* thread_comparisons) {
        ForEachRecommendation(results[index], [&](PairComparisonResult&& comp_res) {
            thread_comparisons->push_back(std::move(comp_res));
        });
    };
    auto infer = [this](PairComparisonResult&& comparison) {
        InferFromComparison(std::move(comparison));
    };
    ParallelCompareAndInfer(compare_records_and_store, results.size(), infer);
}

void RecordPairInferrer::InferFromRecommendationsSeq(
        std::vector<BatchValidator::Result> const& results) {
    for (BatchValidator::Result const& result : results) {
        ForEachRecommendation(result, [&](PairComparisonResult&& comp_res) {
            InferFromComparison(std::move(comp_res));
        });
    }
}

bool RecordPairInferrer::InferFromRecordPairs(std::vector<BatchValidator::Result> const& results) {
    InferFromRecommendations(results);

    bool const nothing_to_sample = SampleAndInfer();
    return nothing_to_sample;
}
}  // namespace algos::hymde::cover_calculation
