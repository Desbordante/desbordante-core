#include "algorithms/md/hymd/record_pair_inferrer.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <compare>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <variant>
#include <vector>

#include <boost/unordered_map.hpp>
#include <easylogging++.h>

#include "algorithms/md/hymd/utility/index_range.h"
#include "desbordante_assume.h"
#include "md/hymd/column_classifier_value_id.h"
#include "md/hymd/indexes/column_similarity_info.h"
#include "md/hymd/indexes/compressed_records.h"
#include "md/hymd/indexes/dictionary_compressor.h"
#include "md/hymd/indexes/keyed_position_list_index.h"
#include "md/hymd/indexes/pli_cluster.h"
#include "md/hymd/indexes/similarity_index.h"
#include "md/hymd/indexes/similarity_matrix.h"
#include "md/hymd/lattice/md_lattice.h"
#include "md/hymd/lhs_ccv_ids_info.h"
#include "md/hymd/lowest_cc_value_id.h"
#include "md/hymd/pair_comparison_result.h"
#include "md/hymd/recommendation.h"
#include "md/hymd/utility/zip.h"
#include "util/get_preallocated_vector.h"
#include "worker_thread_pool.h"

namespace {
algos::hymd::ColumnClassifierValueId GetCCVId(algos::hymd::indexes::SimilarityMatrixRow const& row,
                                              algos::hymd::ValueIdentifier rhs_value) {
    auto ccv_id_it = row.find(rhs_value);
    return ccv_id_it == row.end() ? algos::hymd::kLowestCCValueId : ccv_id_it->second;
}
}  // namespace

namespace algos::hymd {

// NOTE: non-cluster sorting in sampling is disabled because it takes too long for little benefit.
class RecordPairInferrer::RecordRanker {
    using RankingBodyVariables =
            std::tuple<indexes::SimilarityIndex const&, std::vector<indexes::PliCluster> const&>;
    using RankingVariables = std::tuple<std::size_t, RankingBodyVariables>;

    RecordPairInferrer const& inferrer_;

#if 0
    using SamplingOrderStats = std::vector<std::pair<ColumnClassifierValueId, unsigned>>;

    struct FullSamplingSortComparer {
        using StatMap = boost::unordered::unordered_flat_map<RecordIdentifier, SamplingOrderStats>;

        StatMap order_stats_prev;
        StatMap order_stats_next;

        bool operator()(RecordIdentifier rec1, RecordIdentifier rec2) const noexcept;
    };
#endif

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

    template <bool IsShort, typename ObtainValueRecords>
    struct RankRecordsLoopBody;

    std::pair<model::Index, model::Index> GetPrevAndNext(
            model::Index const column_match_index) const noexcept {
        // Prevented in the main part of the algorithm.
        DESBORDANTE_ASSUME(inferrer_.column_match_number_ != 0);
        model::Index const last_cm = inferrer_.column_match_number_ - 1;
        model::Index const prev_column_match_index =
                column_match_index == 0 ? last_cm : column_match_index - 1;
        model::Index const next_column_match_index =
                column_match_index == last_cm ? 0 : column_match_index + 1;
        return {prev_column_match_index, next_column_match_index};
    }

    ShortSamplingClusterComparer CreateShortSamplingClusterComparer(
            model::Index const column_match_index) const noexcept {
        auto const [prev_column_match_index, next_column_match_index] =
                GetPrevAndNext(column_match_index);
        std::vector<CompressedRecord> const& records =
                inferrer_.records_info_->GetLeftCompressor().GetRecords();
        std::vector<ColumnMatchInfo> const& cm_sim_info = *inferrer_.column_matches_sim_info_;

        ColumnMatchInfo const& prev_cm_info = cm_sim_info[prev_column_match_index];
        ColumnMatchInfo const& next_cm_info = cm_sim_info[next_column_match_index];
        ShortSamplingClusterComparer::Info prev_info{prev_cm_info.left_column_index,
                                                     prev_cm_info.right_column_index};
        ShortSamplingClusterComparer::Info next_info{next_cm_info.left_column_index,
                                                     next_cm_info.right_column_index};
        return {records, prev_info, next_info};
    }

    RankingVariables GetRankingVariables(model::Index const column_match_index) {
        ColumnMatchInfo const& column_match_info =
                (*inferrer_.column_matches_sim_info_)[column_match_index];

        indexes::DictionaryCompressor const& left_compressor =
                inferrer_.records_info_->GetLeftCompressor();
        model::Index const left_pli_index = column_match_info.left_column_index;
        std::vector<indexes::PliCluster> const& left_clusters =
                left_compressor.GetPli(left_pli_index).GetClusters();

        indexes::SimilarityIndex const& upper_set_index =
                column_match_info.similarity_info.similarity_index;

        std::size_t const total_left_values = left_clusters.size();
        return {total_left_values, {upper_set_index, left_clusters}};
    }

#if 0
    SamplingOrderStats CreateStats(RecordIdentifier right_record_id,
                                   model::Index column_match_index,
                                   indexes::PliCluster const& cluster) const;
#endif

    template <bool IsShort>
    void RankRecordsSeq(model::Index const column_match_index,
                        RankedRecordsColumnMatch& column_match_ranked_records);
    template <bool IsShort>
    void RankRecordsParallel(model::Index const column_match_index,
                             RankedRecordsColumnMatch& column_match_ranked_records);

#if 0
    static std::strong_ordering CompareRecStats(SamplingOrderStats const& rec1_stats,
                                                SamplingOrderStats const& rec2_stats) noexcept;
#endif

public:
    RecordRanker(RecordPairInferrer const& inferrer) : inferrer_(inferrer) {}

    std::vector<RankedRecordsColumnMatch> RankRecords();
};

#if 0
std::strong_ordering RecordPairInferrer::RecordRanker::CompareRecStats(
        SamplingOrderStats const& rec1_stats, SamplingOrderStats const& rec2_stats) noexcept {
    if (rec1_stats.size() > rec2_stats.size()) {
        auto rec1_stats_it = rec1_stats.begin();
        for (auto const& [ccv_id2, rec_number2] : rec2_stats) {
            auto const& [ccv_id1, rec_number1] = *rec1_stats_it++;
            if (ccv_id1 > ccv_id2) {
                return std::strong_ordering::less;
            } else if (ccv_id1 < ccv_id2) {
                return std::strong_ordering::greater;
            } else if (rec_number1 > rec_number2) {
                return std::strong_ordering::less;
            } else if (rec_number1 < rec_number2) {
                return std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::less;
    }
    auto rec2_stats_it = rec2_stats.begin();
    for (auto const& [ccv_id1, rec_number1] : rec1_stats) {
        auto const& [ccv_id2, rec_number2] = *rec2_stats_it++;
        if (ccv_id1 > ccv_id2) {
            return std::strong_ordering::less;
        } else if (ccv_id1 < ccv_id2) {
            return std::strong_ordering::greater;
        } else if (rec_number1 > rec_number2) {
            return std::strong_ordering::less;
        } else if (rec_number1 < rec_number2) {
            return std::strong_ordering::greater;
        }
    }
    if (rec2_stats_it != rec2_stats.end()) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

auto RecordPairInferrer::RecordRanker::CreateStats(
        RecordIdentifier right_record_id, model::Index column_match_index,
        indexes::PliCluster const& cluster) const -> SamplingOrderStats {
    std::unordered_map<ColumnClassifierValueId, unsigned> stats;
    ColumnMatchInfo const& cm_info = (*column_matches_sim_info_)[column_match_index];
    indexes::SimilarityMatrix const& sim_matrix = cm_info.similarity_info.similarity_matrix;
    std::vector<ColumnClassifierValueId> const& rhs_lhs_map =
            (*lhs_ccv_id_info_)[column_match_index].rhs_to_lhs_map;
    model::Index const left_pli_index = cm_info.left_column_index;
    model::Index const right_pli_index = cm_info.right_column_index;
    ValueIdentifier const right_value_id =
            records_info_->GetRightCompressor().GetRecords()[right_record_id][right_pli_index];
    std::vector<CompressedRecord> const& left_records =
            records_info_->GetLeftCompressor().GetRecords();
    for (RecordIdentifier left_record_id : cluster) {
        ValueIdentifier const left_value_id = left_records[left_record_id][left_pli_index];
        indexes::SimilarityMatrixRow const& row = sim_matrix[left_value_id];
        auto it = row.find(right_value_id);
        if (it == row.end()) continue;
        ColumnClassifierValueId lhs_ccv_id = rhs_lhs_map[it->second];
        if (lhs_ccv_id == kLowestCCValueId) continue;
        ++stats[lhs_ccv_id];
    }
    SamplingOrderStats stats_flat{stats.begin(), stats.end()};
    std::ranges::sort(stats_flat);
    return stats_flat;
}

// Full sort: compare every record pairwise with the cluster's records, sort every CCV ID span
// lexicographically based on the sorted list of CCV IDs in the left column match, then the right
// column match as tie breaker.
bool RecordPairInferrer::RecordRanker::FullSamplingSortComparer::operator()(
        RecordIdentifier record_id1, RecordIdentifier record_id2) const noexcept {
    SamplingOrderStats const& rec1_prev_stats = order_stats_prev.find(record_id1)->second;
    SamplingOrderStats const& rec2_prev_stats = order_stats_prev.find(record_id2)->second;
    std::strong_ordering prev_res = CompareRecStats(rec1_prev_stats, rec2_prev_stats);
    if (prev_res != std::strong_ordering::equal) return prev_res == std::strong_ordering::less;

    SamplingOrderStats const& rec1_next_stats = order_stats_next.find(record_id1)->second;
    SamplingOrderStats const& rec2_next_stats = order_stats_next.find(record_id2)->second;
    std::strong_ordering next_res = CompareRecStats(rec1_next_stats, rec2_next_stats);
    if (next_res != std::strong_ordering::equal) return next_res == std::strong_ordering::less;
    return record_id1 < record_id2;
}
#endif

// Using the sorting from HyFD. It can work okay here if equality implies highest CCV ID, which is
// the case for all built-in similarity measures.
bool RecordPairInferrer::RecordRanker::ShortSamplingClusterComparer::operator()(
        RecordIdentifier record_id1, RecordIdentifier record_id2) const noexcept {
    CompressedRecord const& record1 = records[record_id1];
    CompressedRecord const& record2 = records[record_id2];
    for (model::Index column_index : {prev_info.left_pli_index, prev_info.right_pli_index,
                                      next_info.left_pli_index, next_info.right_pli_index}) {
        auto value_id_order = record1[column_index] <=> record2[column_index];
        if (value_id_order != std::strong_ordering::equal)
            return value_id_order == std::strong_ordering::less;
    }
    return false;
}

//  Short sampling uses a sliding window for sampling cluster records.
//  Short sort, ideally: put the records in such an order that the sorted list of CCV IDs from the
// previous column match is the greatest lexicographically for the first sampling round (window_size
// = 1), if equal, same for the subsequent rounds until not equal. If all equal, sort in the same
// way for the right column match. Apply the above to every span of the same CCV ID in the current
// column match. This is the desired order, but finding it is NP-hard.
//  Instead, this adapts HyFD's approach. The cluster's records are stored first, sorted in a
// similar way to HyFD, followed by other records, sorted by their CCV ID in column_match_index.
//  Full sorting is not implemented.
template <typename ObtainValueRecords>
struct RecordPairInferrer::RecordRanker::RankRecordsLoopBody<true, ObtainValueRecords> {
    RecordRanker const& ranker;
    RankingBodyVariables const variables;
    ObtainValueRecords obtain_value_records;
    model::Index const column_match_index;

    // Sorting the cluster in a way that the first round of sampling yields the most of the highest
    // CCV IDs is an NP-hard problem: an algorithm obtaining such a permutation can be used for
    // solving the Hamiltonian path problem, which is NP-complete.
    void OrderCluster(std::vector<RecordIdentifier>& cluster_records) const noexcept {
        ShortSamplingClusterComparer comparer =
                ranker.CreateShortSamplingClusterComparer(column_match_index);
        std::ranges::sort(cluster_records, comparer);
    }

    void operator()(ValueIdentifier left_value_id) {
        auto const [upper_set_index, left_clusters] = variables;
        auto const& [right_records, end_id_map] = upper_set_index[left_value_id].GetFlat();

        RankedRecordsValue& value_ranked_records = obtain_value_records(left_value_id);
        value_ranked_records.reserve(right_records.size());

        indexes::PliCluster const& cluster = left_clusters[left_value_id];
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
    model::Index const column_match_index;

    void operator()(ValueIdentifier left_value_id) {
        auto const [upper_set_index, left_clusters] = variables;
        auto const& [right_records, end_id_map] = upper_set_index[left_value_id].GetFlat();

        RankedRecordsValue& value_ranked_records = obtain_value_records(left_value_id);
        value_ranked_records.reserve(right_records.size());

        value_ranked_records.insert(value_ranked_records.end(), right_records.begin(),
                                    right_records.end());
#if 0
        indexes::PliCluster const& cluster = left_clusters[left_value_id];
        FullSamplingSortComparer comparer =
                ranker.CreateComparer(cluster, value_ranked_records, column_match_index);
        DESBORDANTE_ASSUME(!end_id_map.empty());
        auto fol_iter = end_id_map.begin(), cur_iter = fol_iter++;
        auto const begin_iter = value_ranked_records.begin();
        std::sort(begin_iter, begin_iter + cur_iter->second, comparer);
        for (auto fol_iter = end_id_map.begin(), cur_iter = fol_iter++, end_iter = end_id_map.end();
             fol_iter != end_iter; ++fol_iter, ++cur_iter) {
            std::sort(begin_iter + cur_iter->second, begin_iter + fol_iter->second, comparer);
        }
#endif
    }
};

template <bool IsShort>
void RecordPairInferrer::RecordRanker::RankRecordsParallel(
        model::Index const column_match_index,
        RankedRecordsColumnMatch& column_match_ranked_records) {
    auto const [total_left_values, variables] = GetRankingVariables(column_match_index);
    // The table is not empty, so there are values.
    DESBORDANTE_ASSUME(total_left_values != 0);

    column_match_ranked_records.assign(total_left_values, {});
    auto obtain_at_index = [&](ValueIdentifier value_id) -> decltype(auto) {
        return column_match_ranked_records[value_id];
    };
    auto loop_body = RankRecordsLoopBody<IsShort, decltype(obtain_at_index)>{
            *this, variables, obtain_at_index, column_match_index};
    auto rank_for = [&](ValueIdentifier left_value_id) { loop_body(left_value_id); };
    inferrer_.pool_->ExecIndex(rank_for, total_left_values);
}

template <bool IsShort>
void RecordPairInferrer::RecordRanker::RankRecordsSeq(
        model::Index const column_match_index,
        RankedRecordsColumnMatch& column_match_ranked_records) {
    auto const [total_left_values, variables] = GetRankingVariables(column_match_index);
    // The table is not empty, so there are values.
    DESBORDANTE_ASSUME(total_left_values != 0);

    auto obtain_new = [&](model::Index) -> decltype(auto) {
        return column_match_ranked_records.emplace_back();
    };
    auto loop_body = RankRecordsLoopBody<IsShort, decltype(obtain_new)>{
            *this, variables, obtain_new, column_match_index};
    for (ValueIdentifier left_value_id : utility::IndexRange(total_left_values)) {
        loop_body(left_value_id);
    }
}

auto RecordPairInferrer::RecordRanker::RankRecords() -> std::vector<RankedRecordsColumnMatch> {
    std::vector<RankedRecordsColumnMatch> ranked_records =
            util::GetPreallocatedVector<RankedRecordsColumnMatch>(inferrer_.column_match_number_);

    using InitMethod = void (RecordRanker::*)(model::Index const, RankedRecordsColumnMatch&);
    InitMethod short_method = &RecordRanker::RankRecordsSeq<true>;
    InitMethod full_method = &RecordRanker::RankRecordsSeq<false>;
    if (inferrer_.MultiThreaded()) {
        short_method = &RecordRanker::RankRecordsParallel<true>;
        full_method = &RecordRanker::RankRecordsParallel<false>;
    }

    for (model::Index const column_match_index :
         utility::IndexRange(inferrer_.column_match_number_)) {
        RankedRecordsColumnMatch& column_match_ranked_records = ranked_records.emplace_back();
        // See CreateSamplingQueue comments.
        if ((*inferrer_.lhs_ccv_id_info_)[column_match_index].lhs_to_rhs_map.size() == 1) continue;

        InitMethod fill_records =
                inferrer_.ShortSamplingEnabled(column_match_index) ? short_method : full_method;
        (this->*fill_records)(column_match_index, column_match_ranked_records);
    }

    return ranked_records;
}

auto RecordPairInferrer::CreateSamplingQueue() -> std::priority_queue<ColumnMatchSamplingInfo> {
    std::vector<ColumnMatchSamplingInfo> sampling_info =
            util::GetPreallocatedVector<ColumnMatchSamplingInfo>(column_match_number_);
    for (model::Index column_match_index : utility::IndexRange(column_match_number_)) {
        auto const& [lhs_rhs_map, rhs_lhs_map] = (*lhs_ccv_id_info_)[column_match_index];
        // TODO: enforce this invariant with a class.
        DESBORDANTE_ASSUME(!lhs_rhs_map.empty());
        // No records are matched.
        if (lhs_rhs_map.size() == 1) {
            // Trivial column matches have been excluded.
            DESBORDANTE_ASSUME(rhs_lhs_map.size() > 1);
            // TODO: sort based on column matches where the number of LHS values is greater
            // than 1, discard pairs with the highest CCV ID.
            LOG(WARNING) << "Sampling for column match " << column_match_index
                         << " not implemented.";
            continue;
        }

        std::size_t const initial_parameter = ShortSamplingEnabled(column_match_index) ? 1 : 0;
        ColumnMatchSamplingInfo& column_match_sampling_info =
                sampling_info.emplace_back(column_match_index, initial_parameter);
        DoSamplingRound(column_match_sampling_info);
        // No pairs have been sampled, and no pairs are going to be sampled if the parameter is
        // increased either.
        if (column_match_sampling_info.GetSampledNumber() == 0) sampling_info.pop_back();
    }
    return std::priority_queue<ColumnMatchSamplingInfo>{decltype(sampling_queue_)::value_compare{},
                                                        std::move(sampling_info)};
}

RecordPairInferrer::RecordPairInferrer(InternalConstructToken, lattice::MdLattice* lattice,
                                       indexes::RecordsInfo const* records_info,
                                       std::vector<ColumnMatchInfo> const* column_matches_sim_info,
                                       std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info,
                                       std::vector<bool> sample_short, util::WorkerThreadPool* pool)
    : lattice_(lattice),
      records_info_(records_info),
      column_matches_sim_info_(column_matches_sim_info),
      lhs_ccv_id_info_(&lhs_ccv_id_info),
      sample_short_(std::move(sample_short)),
      pool_(pool),
      ranked_records_(RecordRanker{*this}.RankRecords()),
      sampling_queue_(CreateSamplingQueue()) {}

PairComparisonResult RecordPairInferrer::CompareRecords(
        CompressedRecord const& left_record, CompressedRecord const& right_record) const {
    std::vector<ColumnClassifierValueId> rhss =
            util::GetPreallocatedVector<ColumnClassifierValueId>(column_match_number_);
    for (auto const& [sim_info, left_col_index, right_col_index] : *column_matches_sim_info_) {
        indexes::SimilarityMatrixRow const& row =
                sim_info.similarity_matrix[left_record[left_col_index]];
        rhss.push_back(GetCCVId(row, right_record[right_col_index]));
    }
    return {std::move(rhss), *lhs_ccv_id_info_};
}

bool RecordPairInferrer::InferFromNew(PairComparisonResult const& pair_comparison_result) {
    using MdRefiner = lattice::MdLattice::MdRefiner;
    std::vector<MdRefiner> refiners = lattice_->CollectRefinersForViolated(pair_comparison_result);
    std::for_each(refiners.begin(), refiners.end(), std::mem_fn(&MdRefiner::Refine));
    return !refiners.empty();
}

template <bool ShortSampling>
class RecordPairInferrer::Sampler {
    indexes::CompressedRecords const& left_records_;
    indexes::CompressedRecords const& right_records_;
    std::vector<indexes::PliCluster> const& clusters_;
    RankedRecordsColumnMatch const& ranked_records_column_match_;
    model::Index const parameter_;

    void SampleWholeCluster(indexes::PliCluster const& cluster,
                            RankedRecordsValue const& ranked_records_value,
                            auto&& record_pair_action) const {
        std::size_t const record_rank = parameter_;

        CompressedRecord const& right_record = right_records_[ranked_records_value[record_rank]];
        for (RecordIdentifier left_record_id : cluster) {
            CompressedRecord const& left_record = left_records_[left_record_id];
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

        for (auto const [left_record_id, right_record_id] : utility::Zip(end, pivot, partner)) {
            CompressedRecord const& right_record = right_records_[right_record_id];
            CompressedRecord const& left_record = left_records_[left_record_id];
            record_pair_action(left_record, right_record);
        }
    }

    void SampleShort(indexes::PliCluster const& cluster,
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
    Sampler(indexes::CompressedRecords const& left_records,
            indexes::CompressedRecords const& right_records,
            std::vector<indexes::PliCluster> const& clusters,
            RankedRecordsColumnMatch const& ranked_records_column_match, model::Index parameter)
        : left_records_(left_records),
          right_records_(right_records),
          clusters_(clusters),
          ranked_records_column_match_(ranked_records_column_match),
          parameter_(parameter) {}

    void Sample(ValueIdentifier value_id, auto&& record_pair_action) const {
        RankedRecordsValue const& ranked_records_value = ranked_records_column_match_[value_id];
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
auto RecordPairInferrer::CreateSampler(ColumnMatchSamplingInfo const& column_match_sampling_info)
        -> std::pair<std::size_t, Sampler<SampleShort>> {
    indexes::DictionaryCompressor const& left_compressor = records_info_->GetLeftCompressor();
    indexes::CompressedRecords const& left_records = left_compressor.GetRecords();

    indexes::CompressedRecords const& right_records =
            records_info_->GetRightCompressor().GetRecords();

    model::Index const column_match_index = column_match_sampling_info.GetColumnMatchIndex();

    ColumnMatchInfo const& column_match_info = (*column_matches_sim_info_)[column_match_index];
    model::Index const left_pli_index = column_match_info.left_column_index;
    indexes::KeyedPositionListIndex const& left_pli = left_compressor.GetPli(left_pli_index);
    std::vector<indexes::PliCluster> const& clusters = left_pli.GetClusters();

    RankedRecordsColumnMatch const& ranked_column_match_records =
            ranked_records_[column_match_index];

    model::Index const parameter = column_match_sampling_info.GetParameter();

    std::size_t const left_values_number = clusters.size();

    return {left_values_number,
            {left_records, right_records, clusters, ranked_column_match_records, parameter}};
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
        ColumnMatchSamplingInfo& column_match_sampling_info) {
    auto const [left_values_number, sampler] = CreateSampler<IsShort>(column_match_sampling_info);

    auto compare_and_store_all_for_value = [this, &sampler](ValueIdentifier left_value_id,
                                                            Comparisons* thread_comparisons) {
        auto compare_and_store = [this, thread_comparisons](CompressedRecord const& left_record,
                                                            CompressedRecord const& right_record) {
            thread_comparisons->push_back(CompareRecords(left_record, right_record));
        };
        sampler.Sample(left_value_id, compare_and_store);
    };

    auto infer_from_compared = [&](PairComparisonResult&& comparison) {
        CountAndInfer(column_match_sampling_info, std::move(comparison));
    };
    ParallelCompareAndInfer(compare_and_store_all_for_value, left_values_number,
                            infer_from_compared);
}

template <bool IsShort>
void RecordPairInferrer::DoSamplingRoundSeq(ColumnMatchSamplingInfo& column_match_sampling_info) {
    auto const [left_values_number, sampler] = CreateSampler<IsShort>(column_match_sampling_info);

    auto infer_immediately = [&](CompressedRecord const& left_record,
                                 CompressedRecord const& right_record) {
        CountAndInfer(column_match_sampling_info, CompareRecords(left_record, right_record));
    };

    for (ValueIdentifier left_value_id : utility::IndexRange(left_values_number)) {
        sampler.Sample(left_value_id, infer_immediately);
    }
}

void RecordPairInferrer::SampleAndRequeue(ColumnMatchSamplingInfo& column_match_sampling_info) {
    std::size_t const sampled_previously = column_match_sampling_info.GetSampledNumber();
    DoSamplingRound(column_match_sampling_info);
    std::size_t const sampled_after_round = column_match_sampling_info.GetSampledNumber();
    bool const out_of_pairs = sampled_previously == sampled_after_round;
    if (out_of_pairs) return;

    sampling_queue_.push(column_match_sampling_info);
}

bool RecordPairInferrer::SampleAndInfer() {
    // Method should not be called if the queue is empty, signaled by the return value of Create and
    // InferFromRecordPairs.
    DESBORDANTE_ASSUME(!sampling_queue_.empty());
    ColumnMatchSamplingInfo best = sampling_queue_.top();
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

void RecordPairInferrer::InferFromRecommendationsParallel(Recommendations const& recommendations) {
    auto compare_records_and_store = [this, &recommendations](model::Index index,
                                                              Comparisons* thread_comparisons) {
        auto const& [left_record_ptr, right_record_ptr] = recommendations[index];
        thread_comparisons->push_back(CompareRecords(*left_record_ptr, *right_record_ptr));
    };
    auto infer = [this](PairComparisonResult&& comparison) {
        InferFromComparison(std::move(comparison));
    };
    ParallelCompareAndInfer(compare_records_and_store, recommendations.size(), infer);
}

void RecordPairInferrer::InferFromRecommendationsSeq(Recommendations const& recommendations) {
    for (auto const& [left_record_ptr, right_record_ptr] : recommendations) {
        InferFromComparison(CompareRecords(*left_record_ptr, *right_record_ptr));
    }
}

bool RecordPairInferrer::InferFromRecordPairs(Recommendations const& recommendations) {
    InferFromRecommendations(recommendations);

    bool const nothing_to_sample = SampleAndInfer();
    return nothing_to_sample;
}

}  // namespace algos::hymd
