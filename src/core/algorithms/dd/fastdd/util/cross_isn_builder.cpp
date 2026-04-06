#include "core/algorithms/dd/fastdd/util/cross_isn_builder.h"

#include <algorithm>
#include <utility>

#include "core/algorithms/dd/dd.h"

namespace algos::dd {

boost::unordered::unordered_flat_set<std::size_t> CrossISNBuilder::BuildISNs() {
    std::size_t num_tuple_pairs = first_pli_shard_.Range() * second_pli_shard_.Range();
    clues_.resize(num_tuple_pairs, 0UL);
    std::vector<DFPack> const& df_packs = isn_info_->GetDFPacks();
    for (auto const& df_pack : df_packs) {
        if (!df_pack.is_distance_ordered) {
            BuildNotDistanceOrdered(df_pack);
        } else {
            BuildDistanceOrdered(df_pack);
        }
    }

    return boost::unordered::unordered_flat_set<std::size_t>(clues_.begin(), clues_.end());
}

void CrossISNBuilder::SetNumMask(Cluster const& first_cluster, Cluster const& second_cluster,
                                 std::size_t const base, std::size_t const offset) {
    std::size_t const diff = base * offset;
    std::size_t const first_beg = first_pli_shard_.Beg();
    std::size_t const second_beg = second_pli_shard_.Beg();
    std::size_t const second_range = second_pli_shard_.Range();
    for (std::size_t first_tuple_index : first_cluster) {
        std::size_t const first_offset = first_tuple_index - first_beg;
        std::size_t const second_offset = first_offset * second_range - second_beg;
        for (std::size_t second_tuple_index : second_cluster) {
            clues_[second_offset + second_tuple_index] += diff;
        }
    }
}

std::vector<std::size_t> CrossISNBuilder::CalculateOffsets(
        DFPack const& df_pack, std::size_t const first_cluster_num) const {
    Pli const& first_pli = first_pli_shard_.Plis()[df_pack.column_index];
    Pli const& second_pli = second_pli_shard_.Plis()[df_pack.column_index];
    std::vector<ThresholdInfo> const& thresholds = df_pack.thresholds;
    std::vector<std::size_t> offsets(second_pli.Size());
    std::size_t start_cluster_num = 0;

    // I'm not sure if this iteration works correctly
    std::size_t left_bound = 0;
    std::size_t right_bound = second_pli.Size() + 1;
    while (right_bound - left_bound > 1) {
        std::size_t const mid = (left_bound + right_bound) / 2;
        model::CompareResult comp_res = distance_calculator_->Compare(
                df_pack.column_index,
                {second_pli.GetCluster(mid - 1)[0], first_pli.GetCluster(first_cluster_num)[0]});
        if (comp_res == model::CompareResult::kLess) {
            right_bound = mid;
        } else {
            left_bound = mid;
        }
    }
    std::size_t const last_cluster_num = right_bound - 1;
    // I'm not sure if this iteration works correctly
    for (std::size_t i = thresholds.size(); i != 0 && start_cluster_num != second_pli.Size(); --i) {
        std::size_t left_bound = start_cluster_num;
        std::size_t right_bound = last_cluster_num + 1;
        while (right_bound - left_bound > 1) {
            std::size_t const mid = (left_bound + right_bound) / 2;
            double const diff = distance_calculator_->CalculateDistance(
                    df_pack.column_index, {first_pli.GetCluster(first_cluster_num)[0],
                                           second_pli.GetCluster(mid - 1)[0]});
            if (ThresholdInfo{diff, true, thresholds.size()} <= thresholds[i - 1]) {
                right_bound = mid;
            } else {
                left_bound = mid;
            }
        }
        std::size_t const end_cluster_num = right_bound - 1;
        for (std::size_t j = start_cluster_num; j != end_cluster_num; ++j) {
            offsets[j] = i;
        }
        start_cluster_num = end_cluster_num;
    }
    if (start_cluster_num == second_pli.Size()) {
        return offsets;
    }

    for (std::size_t i = 0; i != thresholds.size() && start_cluster_num != second_pli.Size(); ++i) {
        std::size_t left_bound = start_cluster_num;
        std::size_t right_bound = second_pli.Size() + 1;
        while (right_bound - left_bound > 1) {
            std::size_t const mid = (left_bound + right_bound) / 2;
            double const diff = distance_calculator_->CalculateDistance(
                    df_pack.column_index, {first_pli.GetCluster(first_cluster_num)[0],
                                           second_pli.GetCluster(mid - 1)[0]});
            if (ThresholdInfo{diff, true, thresholds.size()} > thresholds[i]) {
                right_bound = mid;
            } else {
                left_bound = mid;
            }
        }
        std::size_t const end_cluster_num = right_bound - 1;
        for (std::size_t j = start_cluster_num; j != end_cluster_num; ++j) {
            offsets[j] = i;
        }
        start_cluster_num = end_cluster_num;
    }

    for (std::size_t j = start_cluster_num; j != second_pli.Size(); ++j) {
        offsets[j] = thresholds.size();
    }

    return offsets;
}

void CrossISNBuilder::BuildNotDistanceOrdered(DFPack const& df_pack) {
    Pli const& first_pli = first_pli_shard_.Plis()[df_pack.column_index];
    Pli const& second_pli = second_pli_shard_.Plis()[df_pack.column_index];
    std::vector<ThresholdInfo> const& thresholds = df_pack.thresholds;
    for (std::size_t i = 0; i != first_pli.Size(); ++i) {
        for (std::size_t j = 0; j != second_pli.Size(); ++j) {
            double const diff = distance_calculator_->CalculateDistance(
                    df_pack.column_index,
                    {first_pli.GetCluster(i)[0], second_pli.GetCluster(j)[0]});
            std::size_t const interval_num =
                    std::lower_bound(thresholds.begin(), thresholds.end(),
                                     ThresholdInfo{diff, true, thresholds.size()}) -
                    thresholds.begin();
            SetNumMask(first_pli.GetCluster(i), second_pli.GetCluster(j), df_pack.base,
                       df_pack.threshold_zones[interval_num]);
        }
    }
}

void CrossISNBuilder::BuildDistanceOrdered(DFPack const& df_pack) {
    Pli const& first_pli = first_pli_shard_.Plis()[df_pack.column_index];
    Pli const& second_pli = second_pli_shard_.Plis()[df_pack.column_index];
    for (std::size_t i = 0; i != first_pli.Size(); ++i) {
        std::vector<std::size_t> const offsets = CalculateOffsets(df_pack, i);
        for (std::size_t j = 0; j != second_pli.Size(); ++j) {
            SetNumMask(first_pli.GetCluster(i), second_pli.GetCluster(j), df_pack.base,
                       df_pack.threshold_zones[offsets[j]]);
        }
    }
}

}  // namespace algos::dd
