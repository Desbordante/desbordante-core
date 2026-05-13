#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/algorithms/dd/fastdd/util/isn_info.h"

namespace algos::dd {

template <typename ClueType, BoostDynamicBitsetCompatible Bitset>
class SingleISNBuilder {
private:
    std::shared_ptr<ISNInfo> isn_info_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    PliShard const& pli_shard_;
    std::vector<std::vector<Bitset>> const& offset_to_predicates_;
    std::vector<model::DFConstraint>& min_max_dif_;
    std::size_t bitset_size_;
    std::vector<ClueType> clues_;

    void BuildNotDistanceOrdered(DFPack const& df_pack, std::size_t df_pack_index) {
        Pli const& pli = pli_shard_.Plis()[df_pack.column_index];
        std::vector<ThresholdInfo> const& thresholds = df_pack.thresholds;
        for (std::size_t i = 0; i != pli.Size(); ++i) {
            if (pli.GetCluster(i).size() > 1) {
                min_max_dif_[df_pack.column_index].lower_bound =
                        std::min(min_max_dif_[df_pack.column_index].lower_bound, 0.0);
                std::size_t const interval_num =
                        std::lower_bound(thresholds.begin(), thresholds.end(),
                                         ThresholdInfo{0.0, true, thresholds.size()}) -
                        thresholds.begin();
                std::size_t const offset = df_pack.threshold_zones[interval_num];
                SetNumMask(pli.GetCluster(i), df_pack.base, offset,
                           offset_to_predicates_[df_pack_index][offset]);
            }

            for (std::size_t j = i + 1; j != pli.Size(); ++j) {
                double const diff = distance_calculator_->CalculateDistance(
                        df_pack.column_index, {pli.GetCluster(i)[0], pli.GetCluster(j)[0]});
                min_max_dif_[df_pack.column_index].lower_bound =
                        std::min(min_max_dif_[df_pack.column_index].lower_bound, diff);
                min_max_dif_[df_pack.column_index].upper_bound =
                        std::max(min_max_dif_[df_pack.column_index].upper_bound, diff);
                std::size_t const interval_num =
                        std::lower_bound(thresholds.begin(), thresholds.end(),
                                         ThresholdInfo{diff, true, thresholds.size()}) -
                        thresholds.begin();
                std::size_t const offset = df_pack.threshold_zones[interval_num];
                SetNumMask(pli.GetCluster(i), pli.GetCluster(j), df_pack.base, offset,
                           offset_to_predicates_[df_pack_index][offset]);
            }
        }
    }

    void BuildDistanceOrdered(DFPack const& df_pack, std::size_t df_pack_index) {
        Pli const& pli = pli_shard_.Plis()[df_pack.column_index];
        for (std::size_t i = 0; i != pli.Size(); ++i) {
            if (pli.GetCluster(i).size() > 1) {
                min_max_dif_[df_pack.column_index].lower_bound =
                        std::min(min_max_dif_[df_pack.column_index].lower_bound, 0.0);
                std::size_t const interval_num =
                        std::lower_bound(df_pack.thresholds.begin(), df_pack.thresholds.end(),
                                         ThresholdInfo{0.0, true, df_pack.thresholds.size()}) -
                        df_pack.thresholds.begin();
                std::size_t const offset = df_pack.threshold_zones[interval_num];
                SetNumMask(pli.GetCluster(i), df_pack.base, offset,
                           offset_to_predicates_[df_pack_index][offset]);
            }

            std::vector<std::size_t> const offsets = CalculateOffsets(df_pack, i);
            for (std::size_t j = i + 1; j != pli.Size(); ++j) {
                std::size_t const offset = df_pack.threshold_zones[offsets[j - i - 1]];
                SetNumMask(pli.GetCluster(i), pli.GetCluster(j), df_pack.base, offset,
                           offset_to_predicates_[df_pack_index][offset]);
            }

            if (i != pli.Size() - 1) {
                double const diff = distance_calculator_->CalculateDistance(
                        df_pack.column_index, {pli.GetCluster(i)[0], pli.GetCluster(i + 1)[0]});
                min_max_dif_[df_pack.column_index].lower_bound =
                        std::min(min_max_dif_[df_pack.column_index].lower_bound, diff);
            }
        }
        double const diff = distance_calculator_->CalculateDistance(
                df_pack.column_index, {pli.GetCluster(0)[0], pli.GetCluster(pli.Size() - 1)[0]});
        min_max_dif_[df_pack.column_index].upper_bound =
                std::max(min_max_dif_[df_pack.column_index].upper_bound, diff);
    }

    void SetNumMask(Cluster const& cluster, std::size_t const base, std::size_t const offset,
                    Bitset const& diff_bitset) {
        std::size_t const diff = base * offset;
        for (std::size_t i = 0; i != cluster.size(); ++i) {
            std::size_t const first_tuple_index = cluster[i];
            std::size_t const first_offset = first_tuple_index - pli_shard_.Beg();
            std::size_t const summ = -first_tuple_index - 1 +
                                     first_offset * (2 * pli_shard_.Range() - first_offset - 1) / 2;
            for (std::size_t j = i + 1; j != cluster.size(); ++j) {
                if constexpr (std::is_same_v<ClueType, std::size_t>) {
                    clues_[cluster[j] + summ] += diff;
                } else {
                    clues_[cluster[j] + summ] |= diff_bitset;
                }
            }
        }
    }

    void SetNumMask(Cluster const& first_cluster, Cluster const& second_cluster,
                    std::size_t const base, std::size_t const offset, Bitset const& diff_bitset) {
        std::size_t const diff = base * offset;
        std::size_t first_index = 0;
        std::size_t second_index = 0;
        while (first_index < first_cluster.size() && second_index < second_cluster.size()) {
            if (first_cluster[first_index] < second_cluster[second_index]) {
                std::size_t const first_tuple_index = first_cluster[first_index];
                std::size_t const first_offset = first_tuple_index - pli_shard_.Beg();
                std::size_t const summ =
                        -first_tuple_index - 1 +
                        first_offset * (2 * pli_shard_.Range() - first_offset - 1) / 2;
                for (std::size_t j = second_index; j != second_cluster.size(); ++j) {
                    if constexpr (std::is_same_v<ClueType, std::size_t>) {
                        clues_[second_cluster[j] + summ] += diff;
                    } else {
                        clues_[second_cluster[j] + summ] |= diff_bitset;
                    }
                }
                ++first_index;
            } else {
                std::size_t const first_tuple_index = second_cluster[second_index];
                std::size_t const first_offset = first_tuple_index - pli_shard_.Beg();
                std::size_t const summ =
                        -first_tuple_index - 1 +
                        first_offset * (2 * pli_shard_.Range() - first_offset - 1) / 2;
                for (std::size_t j = first_index; j != first_cluster.size(); ++j) {
                    if constexpr (std::is_same_v<ClueType, std::size_t>) {
                        clues_[first_cluster[j] + summ] += diff;
                    } else {
                        clues_[first_cluster[j] + summ] |= diff_bitset;
                    }
                }
                ++second_index;
            }
        }
    }

    std::vector<std::size_t> CalculateOffsets(DFPack const& df_pack,
                                              std::size_t const first_cluster_num) const {
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

public:
    SingleISNBuilder(std::shared_ptr<ISNInfo> isn_info,
                     std::shared_ptr<DistanceCalculator> distance_calculator,
                     PliShard const& pli_shard,
                     std::vector<std::vector<Bitset>> const& offset_to_predicates,
                     std::vector<model::DFConstraint>& min_max_dif, std::size_t bitset_size)
        : isn_info_(isn_info),
          distance_calculator_(distance_calculator),
          pli_shard_(pli_shard),
          offset_to_predicates_(offset_to_predicates),
          min_max_dif_(min_max_dif),
          bitset_size_(bitset_size) {}

    boost::unordered::unordered_flat_set<ClueType> BuildISNs() {
        std::size_t num_tuple_pairs = pli_shard_.Range() * (pli_shard_.Range() - 1) / 2;
        if constexpr (std::is_same_v<ClueType, std::size_t>) {
            clues_.resize(num_tuple_pairs, 0UL);
        } else {
            clues_.resize(num_tuple_pairs, ClueType{bitset_size_});
        }
        std::vector<DFPack> const& df_packs = isn_info_->GetDFPacks();
        for (std::size_t index = 0; index != df_packs.size(); ++index) {
            DFPack const& df_pack = df_packs[index];
            if (!df_pack.is_distance_ordered) {
                BuildNotDistanceOrdered(df_pack, index);
            } else {
                BuildDistanceOrdered(df_pack, index);
            }
        }

        return boost::unordered::unordered_flat_set<ClueType>(clues_.begin(), clues_.end());
    }
};

}  // namespace algos::dd
