#include "core/algorithms/dd/fastdd/util/single_isn_builder.h"

#include <algorithm>
#include <utility>

#include "core/algorithms/dd/dd.h"

namespace algos::dd {

std::unordered_set<std::size_t> SingleISNBuilder::BuildISNs() {
    std::size_t num_tuple_pairs = pli_shard_.Range() * (pli_shard_.Range() - 1) / 2;
    clues_.resize(num_tuple_pairs, 0UL);
    std::vector<DFPack> const& df_packs = isn_info_->GetDFPacks();
    for (auto const& df_pack : df_packs) {
        if (!df_pack.is_distance_ordered) {
            BuildNotDistanceOrdered(df_pack);
        } else {
            BuildDistanceOrdered(df_pack);
        }
    }

    return std::unordered_set<std::size_t>(clues_.begin(), clues_.end());
}

void SingleISNBuilder::SetNumMask(Cluster const& first_cluster, Cluster const& second_cluster,
                                  std::size_t const base, std::size_t const offset) {
    std::size_t const diff = base * offset;
    for (std::size_t i = 0; i != first_cluster.size(); ++i) {
        for (std::size_t j = 0; j != second_cluster.size(); ++j) {
            std::size_t first_tuple_index = first_cluster[i];
            std::size_t second_tuple_index = second_cluster[j];
            if (first_tuple_index == second_tuple_index) {
                continue;
            }
            if (first_tuple_index > second_tuple_index) {
                std::swap(first_tuple_index, second_tuple_index);
            }
            std::size_t const first_offset = first_tuple_index - pli_shard_.Beg();
            std::size_t const pair_index =
                    (second_tuple_index - first_tuple_index - 1) +
                    first_offset * (2 * pli_shard_.Range() - first_offset - 1) / 2;
            if (clues_[pair_index] < base) {
                clues_[pair_index] += diff;
            }
        }
    }
}

std::vector<std::size_t> SingleISNBuilder::CalculateOffsets(
        DFPack const& df_pack, std::size_t const first_cluster_num) const {
    Pli const& pli = pli_shard_.Plis()[df_pack.column_index];
    std::vector<ThresholdInfo> const& thresholds = df_pack.thresholds;
    std::vector<std::size_t> offsets(pli.Size() - first_cluster_num - 1);
    std::size_t start_cluster_num = first_cluster_num + 1;
    for (std::size_t i = 0; i != thresholds.size() && start_cluster_num != pli.Size(); ++i) {
        std::size_t left_bound = start_cluster_num - 1;
        std::size_t right_bound = pli.Size();
        while (right_bound - left_bound > 1) {
            std::size_t const mid = (left_bound + right_bound) / 2;
            double const diff = distance_calculator_->CalculateDistance(
                    df_pack.column_index,
                    {pli.GetCluster(first_cluster_num)[0], pli.GetCluster(mid)[0]});
            if (ThresholdInfo{diff, true, thresholds.size()} > thresholds[i]) {
                right_bound = mid;
            } else {
                left_bound = mid;
            }
        }
        std::size_t const end_cluster_num = right_bound;
        for (std::size_t j = start_cluster_num; j != end_cluster_num; ++j) {
            offsets[j - first_cluster_num - 1] = i;
        }
        start_cluster_num = end_cluster_num;
    }

    for (std::size_t j = start_cluster_num; j != pli.Size(); ++j) {
        offsets[j - first_cluster_num - 1] = thresholds.size();
    }

    return offsets;
}

void SingleISNBuilder::BuildNotDistanceOrdered(DFPack const& df_pack) {
    Pli const& pli = pli_shard_.Plis()[df_pack.column_index];
    std::vector<ThresholdInfo> const& thresholds = df_pack.thresholds;
    for (std::size_t i = 0; i != pli.Size(); ++i) {
        if (pli.GetCluster(i).size() > 1) {
            std::size_t const interval_num =
                    std::lower_bound(thresholds.begin(), thresholds.end(),
                                     ThresholdInfo{0.0, true, thresholds.size()}) -
                    thresholds.begin();
            SetNumMask(pli.GetCluster(i), pli.GetCluster(i), df_pack.base,
                       df_pack.threshold_zones[interval_num]);
        }

        for (std::size_t j = i + 1; j != pli.Size(); ++j) {
            double const diff = distance_calculator_->CalculateDistance(
                    df_pack.column_index, {pli.GetCluster(i)[0], pli.GetCluster(j)[0]});
            std::size_t const interval_num =
                    std::lower_bound(thresholds.begin(), thresholds.end(),
                                     ThresholdInfo{diff, true, thresholds.size()}) -
                    thresholds.begin();
            SetNumMask(pli.GetCluster(i), pli.GetCluster(j), df_pack.base,
                       df_pack.threshold_zones[interval_num]);
        }
    }
}

void SingleISNBuilder::BuildDistanceOrdered(DFPack const& df_pack) {
    Pli const& pli = pli_shard_.Plis()[df_pack.column_index];
    for (std::size_t i = 0; i != pli.Size(); ++i) {
        if (pli.GetCluster(i).size() > 1) {
            std::size_t const interval_num =
                    std::lower_bound(df_pack.thresholds.begin(), df_pack.thresholds.end(),
                                     ThresholdInfo{0.0, true, df_pack.thresholds.size()}) -
                    df_pack.thresholds.begin();
            SetNumMask(pli.GetCluster(i), pli.GetCluster(i), df_pack.base,
                       df_pack.threshold_zones[interval_num]);
        }

        std::vector<std::size_t> const offsets = CalculateOffsets(df_pack, i);
        for (std::size_t j = i + 1; j != pli.Size(); ++j) {
            SetNumMask(pli.GetCluster(i), pli.GetCluster(j), df_pack.base,
                       df_pack.threshold_zones[offsets[j - i - 1]]);
        }
    }
}

}  // namespace algos::dd
