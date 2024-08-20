#include "algorithms/md/hymd/record_pair_inferrer.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

#include <boost/sort/spreadsort/string_sort.hpp>
#include <easylogging++.h>

namespace algos::hymd {

bool RecordPairInferrer::Initialize() {
    std::vector<Efficiency> efficiencies;
    efficiencies.reserve(column_match_number_);
    ranked_records_.assign(column_match_number_, {});

    using InitMethod = bool (RecordPairInferrer::*)(Efficiency&);
    InitMethod short_method = &RecordPairInferrer::InitializeShortSeq;
    InitMethod full_method = &RecordPairInferrer::InitializeFullSeq;
    if (MultiThreaded()) {
        short_method = &RecordPairInferrer::InitializeShortParallel;
        full_method = &RecordPairInferrer::InitializeFullParallel;
    }

    for (model::Index column_match_index = 0; column_match_index != column_match_number_;
         ++column_match_index) {
        auto const& [lhs_rhs_map, rhs_lhs_map] = (*lhs_ccv_id_info_)[column_match_index];
        // TODO: enforce this invariant with a class.
        DESBORDANTE_ASSUME(!lhs_rhs_map.empty());
        if (lhs_rhs_map.size() == 1) {
            DESBORDANTE_ASSUME(rhs_lhs_map.size() > 1);
            // TODO: sort based on column matches where the number of LHS values is greater
            // than 1, discard pairs with the highest CCV ID.
            LOG(WARNING) << "Sampling for column match " << column_match_index
                         << " not implemented.";
            // else No point in sampling.
            continue;
        }
        InitMethod method = full_method;
        std::size_t initial_parameter = 0;
        if (ShortSamplingEnabled(column_match_index)) {
            method = short_method;
            initial_parameter = 1;
        }
        Efficiency& efficiency = efficiencies.emplace_back(column_match_index, initial_parameter);
        if (!(this->*method)(efficiency)) efficiencies.pop_back();
    }
    efficiency_queue_ = std::priority_queue<Efficiency>{
            decltype(efficiency_queue_)::value_compare{}, std::move(efficiencies)};
    return efficiency_queue_.empty();
}

std::strong_ordering RecordPairInferrer::CompareRecStats(
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

auto RecordPairInferrer::CreateStats(
        RecordIdentifier right_record_id, model::Index column_match_index,
        indexes::PliCluster const& cluster) const -> SamplingOrderStats {
    std::unordered_map<ColumnClassifierValueId, unsigned> stats;
    ColumnMatchInfo const& cm_info = (*column_matches_sim_info_)[column_match_index];
    indexes::SimilarityMatrix const& sim_matrix = cm_info.similarity_info.similarity_matrix;
    std::vector<ColumnClassifierValueId> const& rhs_lhs_map =
            (*lhs_ccv_id_info_)[column_match_index].rhs_to_lhs_map;
    model::Index const left_pli_index = cm_info.right_column_index;
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
    std::sort(stats_flat.begin(), stats_flat.end());
    return stats_flat;
}

bool RecordPairInferrer::FullSamplingSortComparer::operator()(
        RecordIdentifier rec1, RecordIdentifier rec2) const noexcept {
    SamplingOrderStats const& rec1_prev_stats = order_stats_prev.find(rec1)->second;
    SamplingOrderStats const& rec2_prev_stats = order_stats_prev.find(rec2)->second;
    std::strong_ordering prev_res = CompareRecStats(rec1_prev_stats, rec2_prev_stats);
    if (prev_res != std::strong_ordering::equal) return prev_res == std::strong_ordering::less;

    SamplingOrderStats const& rec1_next_stats = order_stats_next.find(rec1)->second;
    SamplingOrderStats const& rec2_next_stats = order_stats_next.find(rec2)->second;
    std::strong_ordering next_res = CompareRecStats(rec1_next_stats, rec2_next_stats);
    if (next_res != std::strong_ordering::equal) return next_res == std::strong_ordering::less;
    return rec1 < rec2;
}

bool RecordPairInferrer::ShortSamplingClusterComparer::operator()(
        RecordIdentifier record_id1, RecordIdentifier record_id2) const noexcept {
    CompressedRecord const& record1 = records[record_id1];
    CompressedRecord const& record2 = records[record_id2];
    auto v_comp = [&](model::Index pli_index) { return record1[pli_index] <=> record2[pli_index]; };
    for (model::Index pli_index : {prev_info.left_pli_index, prev_info.right_pli_index,
                                   next_info.left_pli_index, next_info.right_pli_index}) {
        auto comp_res = v_comp(pli_index);
        if (comp_res != std::strong_ordering::equal) return comp_res == std::strong_ordering::less;
    }
    return false;
}

bool RecordPairInferrer::ShortSamplingNonClusterComparer::RecLessThan(
        RecordIdentifier record1, RecordIdentifier record2) const noexcept {
    auto rv = std::ranges::reverse_view{cluster_span};
    auto compare_on = [&](Info info) {
        auto const& [pli_index, rhs_lhs_map, matrix] = info;
        ValueMatrixRowRef rec1_row = matrix[records[record1][pli_index]];
        ValueMatrixRowRef rec2_row = matrix[records[record2][pli_index]];
        for (RecordIdentifier cluster_record_id : rv) {
            ValueIdentifier cur_value_id = records[cluster_record_id][pli_index];
            auto it1 = rec1_row.find(cur_value_id);
            auto it2 = rec2_row.find(cur_value_id);
            ColumnClassifierValueId lhs1_ccv_id =
                    it1 == rec1_row.end() ? kLowestCCValueId : rhs_lhs_map[it1->second];
            ColumnClassifierValueId lhs2_ccv_id =
                    it2 == rec2_row.end() ? kLowestCCValueId : rhs_lhs_map[it2->second];
            if (lhs1_ccv_id != lhs2_ccv_id) return lhs1_ccv_id <=> lhs2_ccv_id;
        }
        return std::strong_ordering::equal;
    };
    auto prev_res = compare_on(prev_info);
    if (prev_res != std::strong_ordering::equal) return prev_res == std::strong_ordering::less;

    auto next_res = compare_on(next_info);
    if (next_res != std::strong_ordering::equal) return next_res == std::strong_ordering::less;
    return record1 < record2;
}

std::uint16_t RecordPairInferrer::ShortSamplingNonClusterComparer::GetBracket(
        RecordIdentifier record_id, model::Index offset) {
    model::Index cluster_record_index = long_block ? offset / 2 : offset;
    indexes::SimilarityMatrix const* matrix = &prev_info.matrix;
    model::Index pli_index = prev_info.pli_index;
    std::vector<ColumnClassifierValueId> const* rhs_lhs_map = &prev_info.rhs_lhs_map;
    if (cluster_record_index >= cluster_size) {
        cluster_record_index -= cluster_size;
        matrix = &next_info.matrix;
        pli_index = next_info.pli_index;
        rhs_lhs_map = &next_info.rhs_lhs_map;
    }
    ValueMatrixRowRef rec_row = (*matrix)[records[record_id][pli_index]];
    auto it = rec_row.find(cluster_span[cluster_size - (cluster_record_index + 1)]);
    return it == rec_row.end() ? kLowestCCValueId : (*rhs_lhs_map)[it->second];
}

// Short sampling uses a sliding window.
// Short sort: put the records in such an order that the sorted list of CCV IDs from the left column
// match is the greatest lexicographically for the first sampling round (window_size = 1), if equal,
// same for the subsequent rounds until not equal. If all equal, sort in the same way for the right
// column. Apply the above to every span of the same CCV ID in the current column match. This is the
// desired order, but establishing is an NP-hard problem.
// Instead, this adapts HyFD's approach. The cluster's records are stored first, sorted in a similar
// way to HyFD, followed by other records, sorted by their CCV ID in column_match_index. The
// non-cluster records in every CCV ID span are sorted lexicographically by the CCV IDs in
// comparisons with the cluster's records (reversed), first on the left column match, then the
// right. That is, if the cluster part is stored as [1 2 3 4 5 6 7], record 8 is in the upper set
// index (aka similarity index) then the value in record 8 is first compared with the value of
// record 7, then 6, then ..., the resulting list for record 8 may look something like
// [3 60 33 5 5 1 9], these lists are compared lexicographically.
template <typename ObtainValueRecords>
struct RecordPairInferrer::InitializeLoopBody<true, ObtainValueRecords> {
    using Info = std::tuple<indexes::SimilarityMatrix const&, model::Index, model::Index,
                            indexes::SimilarityMatrix const&, model::Index, model::Index,
                            std::vector<CompressedRecord> const&>;
    RecordPairInferrer const& inferrer;
    InitializationBodyVariables const variables;
    ObtainValueRecords obtain_value_records;
    model::Index const column_match_index;

    // Sorting the cluster in a way that the first round of sampling yields the most of the highest
    // CCV IDs is an NP-hard problem: an algorithm obtaining such a permutation can be used for
    // solving the Hamiltonian path problem, which is NP-complete.
    void OrderCluster(std::span<RecordIdentifier> cluster_span) const noexcept {
        ShortSamplingClusterComparer comparer =
                inferrer.CreateShortSamplingClusterComparer(column_match_index);
        std::sort(cluster_span.begin(), cluster_span.end(), comparer);
    }

    void operator()(ValueIdentifier left_value_id) {
        auto const [upper_set_index, left_clusters] = variables;
        auto const& [right_records, end_id_map] = upper_set_index[left_value_id].GetFlat();
        ValueRankedRecords& value_ranked_records = obtain_value_records(left_value_id);
        value_ranked_records.reserve(right_records.size());
        indexes::PliCluster const& cluster = left_clusters[left_value_id];
        assert(std::all_of(cluster.begin(), cluster.end(), [&](RecordIdentifier left_record_id) {
            auto right_recs_end = right_records.begin() + end_id_map.begin()->second;
            return std::find(right_records.begin(), right_recs_end, left_record_id) !=
                   right_recs_end;
        }));
        value_ranked_records.insert(value_ranked_records.end(), cluster.begin(), cluster.end());
        std::span cluster_span{value_ranked_records.begin(), value_ranked_records.end()};
        OrderCluster(cluster_span);
        std::unordered_set<RecordIdentifier> cluster_set(cluster.begin(), cluster.end());
        for (RecordIdentifier record_id : right_records) {
            if (!cluster_set.contains(record_id)) value_ranked_records.push_back(record_id);
        }
        /*ShortSamplingNonClusterComparer non_cluster_comparer =
                inferrer.CreateShortSamplingNonClusterComparer(column_match_index, cluster_span);
        DESBORDANTE_ASSUME(!end_id_map.empty());
        auto fol_iter = end_id_map.begin(), cur_iter = fol_iter++;
        auto const begin_iter = value_ranked_records.begin();
        std::size_t const cluster_size = cluster.size();
        auto bracket_getter = non_cluster_comparer.BracketGetter();
        auto size_getter = non_cluster_comparer.SizeGetter();
        auto record_comparer = non_cluster_comparer.RecordComparer();
        boost::sort::spreadsort::string_sort(begin_iter + cluster_size,
                                             begin_iter + cur_iter->second, bracket_getter,
                                             size_getter, record_comparer);
        for (auto fol_iter = end_id_map.begin(), cur_iter = fol_iter++, end_iter = end_id_map.end();
             fol_iter != end_iter; ++fol_iter, ++cur_iter) {
            boost::sort::spreadsort::string_sort(begin_iter + cur_iter->second,
                                                 begin_iter + fol_iter->second, bracket_getter,
                                                 size_getter, record_comparer);
        }*/
    }
};

// Full sort: compare every record pairwise with the cluster's records, sort every CCV ID span
// lexicographically based on the sorted list of CCV IDs in the left column match, then the right
// column match.
template <typename ObtainValueRecords>
struct RecordPairInferrer::InitializeLoopBody<false, ObtainValueRecords> {
    RecordPairInferrer const& inferrer;
    InitializationBodyVariables const variables;
    ObtainValueRecords obtain_value_records;
    model::Index const column_match_index;

    void operator()(ValueIdentifier left_value_id) {
        auto const [upper_set_index, left_clusters] = variables;
        auto const& [right_records, end_id_map] = upper_set_index[left_value_id].GetFlat();
        ValueRankedRecords& value_ranked_records = obtain_value_records(left_value_id);
        value_ranked_records.reserve(right_records.size());
        value_ranked_records.insert(value_ranked_records.end(), right_records.begin(),
                                    right_records.end());
        /*indexes::PliCluster const& cluster = left_clusters[left_value_id];
        FullSamplingSortComparer comparer =
                inferrer.CreateComparer(cluster, value_ranked_records, column_match_index);
        DESBORDANTE_ASSUME(!end_id_map.empty());
        auto fol_iter = end_id_map.begin(), cur_iter = fol_iter++;
        auto const begin_iter = value_ranked_records.begin();
        std::sort(begin_iter, begin_iter + cur_iter->second, comparer);
        for (auto fol_iter = end_id_map.begin(), cur_iter = fol_iter++, end_iter = end_id_map.end();
             fol_iter != end_iter; ++fol_iter, ++cur_iter) {
            std::sort(begin_iter + cur_iter->second, begin_iter + fol_iter->second, comparer);
        }
        */
    }
};

bool RecordPairInferrer::InitializeShortSeq(Efficiency& efficiency) {
    return InitializeSeqImpl<true>(efficiency);
}

bool RecordPairInferrer::InitializeFullSeq(Efficiency& efficiency) {
    return InitializeSeqImpl<false>(efficiency);
}

template <bool IsShort>
bool RecordPairInferrer::InitializeSeqImpl(Efficiency& efficiency) {
    auto const [total_left_values, column_match_ranked_records, variables] =
            GetInitializationVariables(efficiency);
    DESBORDANTE_ASSUME(total_left_values != 0);
    auto obtain_value_records = [&](model::Index) -> decltype(auto) {
        return column_match_ranked_records.emplace_back();
    };
    auto loop_body = InitializeLoopBody<IsShort, decltype(obtain_value_records)>{
            *this, variables, obtain_value_records, efficiency.GetColumnMatchIndex()};
    for (ValueIdentifier left_value_id = 0; left_value_id != total_left_values; ++left_value_id) {
        loop_body(left_value_id);
    }
    SampleSeqImpl<IsShort>(efficiency);
    return efficiency.GetComparisons();
}

template <bool IsShort>
bool RecordPairInferrer::InitializeParallelImpl(Efficiency& efficiency) {
    auto const [total_left_values, column_match_ranked_records, variables] =
            GetInitializationVariables(efficiency);
    DESBORDANTE_ASSUME(total_left_values != 0);
    column_match_ranked_records.assign(total_left_values, {});
    auto obtain_value_records = [&](ValueIdentifier value_id) -> decltype(auto) {
        return column_match_ranked_records[value_id];
    };
    auto loop_body = InitializeLoopBody<IsShort, decltype(obtain_value_records)>{
            *this, variables, obtain_value_records, efficiency.GetColumnMatchIndex()};
    auto fill_for = [&](ValueIdentifier left_value_id) { loop_body(left_value_id); };
    pool_->ExecIndex(fill_for, total_left_values);
    SampleParallelImpl<IsShort>(efficiency);
    return efficiency.GetComparisons();
}

bool RecordPairInferrer::InitializeFullParallel(Efficiency& efficiency) {
    return InitializeParallelImpl<false>(efficiency);
}

bool RecordPairInferrer::InitializeShortParallel(Efficiency& efficiency) {
    return InitializeParallelImpl<true>(efficiency);
}

void RecordPairInferrer::ProcessPairComparison(PairComparisonResult const& pair_comparison_result) {
    using MdRefiner = lattice::MdLattice::MdRefiner;
    std::vector<MdRefiner> refiners = lattice_->CollectRefinersForViolated(pair_comparison_result);
    std::for_each(refiners.begin(), refiners.end(), std::mem_fn(&MdRefiner::Refine));
}

void RecordPairInferrer::ParallelCompareAndProcess(auto compare, std::size_t index_limit,
                                                   auto comparison_action) {
    std::vector<Comparisons> comparisons(pool_->ThreadNum());
    std::atomic<Comparisons*> current_ptr = &comparisons.front();
    auto get_thread_vec = [&]() { return current_ptr.fetch_add(1, std::memory_order::acquire); };
    pool_->ExecIndexWithResource(compare, get_thread_vec, index_limit);
    for (Comparisons& thread_comparisons : comparisons) {
        for (PairComparisonResult& comparison : thread_comparisons) {
            comparison_action(std::move(comparison));
        }
    }
}

void RecordPairInferrer::ProcessRecommendationsParallel(Recommendations recommendations) {
    auto compare_pair = [&](model::Index index, Comparisons* vec) {
        auto const& [left_record_ptr, right_record_ptr] = recommendations[index];
        vec->push_back(CompareRecords(*left_record_ptr, *right_record_ptr));
    };
    auto try_process = [this](PairComparisonResult&& comparison) {
        TryProcessComparison(std::move(comparison));
    };
    ParallelCompareAndProcess(compare_pair, recommendations.size(), try_process);
}

void RecordPairInferrer::ProcessRecommendationsSeq(Recommendations recommendations) {
    for (auto const& [left_record_ptr, right_record_ptr] : recommendations) {
        TryProcessComparison(CompareRecords(*left_record_ptr, *right_record_ptr));
    }
}

template <>
struct RecordPairInferrer::SampleLoopBody<false> {
    SamplingBodyVariables variables;

    template <typename AddComparisonType>
    void operator()(ValueIdentifier value_id, AddComparisonType add_comparison) {
        auto const [left_records, right_records, clusters, ranked_column_match_records,
                    record_rank] = variables;
        ValueRankedRecords const& ranked_list = ranked_column_match_records[value_id];
        if (record_rank >= ranked_list.size()) return;
        CompressedRecord const& right_record = right_records[ranked_list[record_rank]];
        indexes::PliCluster const& cluster = clusters[value_id];
        for (RecordIdentifier left_record_id : cluster) {
            CompressedRecord const& left_record = left_records[left_record_id];
            add_comparison(left_record, right_record);
        }
    }
};

template <>
struct RecordPairInferrer::SampleLoopBody<true> {
    SamplingBodyVariables variables;

    template <typename AddComparisonType>
    void operator()(ValueIdentifier value_id, AddComparisonType add_comparison) {
        auto const [left_records, right_records, clusters, ranked_column_match_records,
                    window_size] = variables;
        ValueRankedRecords const& ranked_list = ranked_column_match_records[value_id];
        if (window_size >= ranked_list.size()) return;
        indexes::PliCluster const& cluster = clusters[value_id];
        DESBORDANTE_ASSUME(ranked_list.size() >= cluster.size());
        std::size_t limit = std::min(ranked_list.size() - window_size, cluster.size());
        auto cluster_it = ranked_list.begin();
        auto cluster_end = ranked_list.begin() + limit;
        auto list_it = ranked_list.begin() + window_size;
        while (cluster_it != cluster_end) {
            RecordIdentifier left_record_id = *cluster_it++;
            RecordIdentifier right_record_id = *list_it++;
            CompressedRecord const& right_record = right_records[right_record_id];
            CompressedRecord const& left_record = left_records[left_record_id];
            add_comparison(left_record, right_record);
        }
    }
};

template <bool IsShort>
void RecordPairInferrer::SampleParallelImpl(Efficiency& efficiency) {
    auto const [clusters_number, variables] = GetSamplingVariables(efficiency);
    auto loop_body = SampleLoopBody<IsShort>{variables};
    auto compare_window = [&](ValueIdentifier value_id, Comparisons* vec) {
        auto compare_and_push_back = [&](CompressedRecord const& left_record,
                                         CompressedRecord const& right_record) {
            vec->push_back(CompareRecords(left_record, right_record));
        };
        loop_body(value_id, compare_and_push_back);
    };
    auto add_comparison = [&](PairComparisonResult&& comparison) {
        AddComparison(efficiency, std::move(comparison));
    };
    ParallelCompareAndProcess(compare_window, clusters_number, add_comparison);
}

void RecordPairInferrer::SampleParallel(Efficiency& efficiency) {
    if (ShortSamplingEnabled(efficiency.GetColumnMatchIndex())) {
        SampleParallelImpl<true>(efficiency);
    } else {
        SampleParallelImpl<false>(efficiency);
    }
}

template <bool IsShort>
void RecordPairInferrer::SampleSeqImpl(Efficiency& efficiency) {
    auto const [clusters_number, variables] = GetSamplingVariables(efficiency);
    auto compare_and_add = [&](CompressedRecord const& left_record,
                               CompressedRecord const& right_record) {
        AddComparison(efficiency, CompareRecords(left_record, right_record));
    };
    auto loop_body = SampleLoopBody<IsShort>{variables};
    for (ValueIdentifier value_id = 0; value_id != clusters_number; ++value_id) {
        loop_body(value_id, compare_and_add);
    }
}

void RecordPairInferrer::SampleSeq(Efficiency& efficiency) {
    if (ShortSamplingEnabled(efficiency.GetColumnMatchIndex())) {
        SampleSeqImpl<true>(efficiency);
    } else {
        SampleSeqImpl<false>(efficiency);
    }
}

PairComparisonResult RecordPairInferrer::CompareRecords(
        CompressedRecord const& left_record, CompressedRecord const& right_record) const {
    std::vector<ColumnClassifierValueId> rhss;
    rhss.reserve(column_match_number_);
    for (auto const& [sim_info, left_col_index, right_col_index] : *column_matches_sim_info_) {
        indexes::SimilarityMatrixRow const& row =
                sim_info.similarity_matrix[left_record[left_col_index]];
        auto sim_it = row.find(right_record[right_col_index]);
        rhss.push_back(sim_it == row.end() ? kLowestCCValueId : sim_it->second);
    }
    return {std::move(rhss), *lhs_ccv_id_info_};
}

bool RecordPairInferrer::InferFromRecordPairs(Recommendations recommendations) {
    (this->*recom_process_method_)(std::move(recommendations));

    constexpr double kTopThresholdDecrease = 0.9;
    DESBORDANTE_ASSUME(!efficiency_queue_.empty());
    Efficiency best_efficiency = efficiency_queue_.top();
    efficiency_queue_.pop();
    efficiency_threshold_ = std::min(efficiency_threshold_ * kNormalThresholdDecrease,
                                     best_efficiency.CalcEfficiency() * kTopThresholdDecrease);

    do {
        std::size_t const prev_comparisons = best_efficiency.GetComparisons();
        (this->*sampling_method_)(best_efficiency);
        if (prev_comparisons != best_efficiency.GetComparisons()) [[likely]] {
            // Something was compared.
            best_efficiency.IncrementParameter();
            efficiency_queue_.push(best_efficiency);
        }
        best_efficiency = efficiency_queue_.top();
        if (best_efficiency.CalcEfficiency() < efficiency_threshold_) return false;
        efficiency_queue_.pop();
    } while (!efficiency_queue_.empty());
    return true;
}

}  // namespace algos::hymd
