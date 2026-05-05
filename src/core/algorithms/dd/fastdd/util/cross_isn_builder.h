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
class CrossISNBuilder {
private:
    std::shared_ptr<ISNInfo> isn_info_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    PliShard const& first_pli_shard_;
    PliShard const& second_pli_shard_;
    std::vector<std::vector<Bitset>> const& offset_to_predicates_;
    std::size_t bitset_size_;
    std::vector<ClueType> clues_;

    void BuildNotDistanceOrdered(DFPack const& df_pack, std::size_t df_pack_index) {
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
                std::size_t const offset = df_pack.threshold_zones[interval_num];
                SetNumMask(first_pli.GetCluster(i), second_pli.GetCluster(j), df_pack.base, offset,
                           offset_to_predicates_[df_pack_index][offset]);
            }
        }
    }

    void BuildDistanceOrdered(DFPack const& df_pack, std::size_t df_pack_index) {
        Pli const& first_pli = first_pli_shard_.Plis()[df_pack.column_index];
        Pli const& second_pli = second_pli_shard_.Plis()[df_pack.column_index];
        for (std::size_t i = 0; i != first_pli.Size(); ++i) {
            std::vector<std::size_t> const offsets = CalculateOffsets(df_pack, i);
            for (std::size_t j = 0; j != second_pli.Size(); ++j) {
                std::size_t const offset = df_pack.threshold_zones[offsets[j]];
                SetNumMask(first_pli.GetCluster(i), second_pli.GetCluster(j), df_pack.base, offset,
                           offset_to_predicates_[df_pack_index][offset]);
            }
        }
    }

    void SetNumMask(Cluster const& first_cluster, Cluster const& second_cluster,
                    std::size_t const base, std::size_t const offset, Bitset const& diff_bitset) {
        std::size_t const diff = base * offset;
        std::size_t const first_beg = first_pli_shard_.Beg();
        std::size_t const second_beg = second_pli_shard_.Beg();
        std::size_t const second_range = second_pli_shard_.Range();
        for (std::size_t first_tuple_index : first_cluster) {
            std::size_t const first_offset = first_tuple_index - first_beg;
            std::size_t const second_offset = first_offset * second_range - second_beg;
            for (std::size_t second_tuple_index : second_cluster) {
                if constexpr (std::is_same_v<ClueType, std::size_t>) {
                    clues_[second_offset + second_tuple_index] += diff;
                } else {
                    clues_[second_offset + second_tuple_index] |= diff_bitset;
                }
            }
        }
    }

    std::vector<std::size_t> CalculateOffsets(DFPack const& df_pack,
                                              std::size_t const first_cluster_num) const {
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
                    df_pack.column_index, {second_pli.GetCluster(mid - 1)[0],
                                           first_pli.GetCluster(first_cluster_num)[0]});
            if (comp_res == model::CompareResult::kLess) {
                right_bound = mid;
            } else {
                left_bound = mid;
            }
        }
        std::size_t const last_cluster_num = right_bound - 1;
        // I'm not sure if this iteration works correctly
        for (std::size_t i = thresholds.size(); i != 0 && start_cluster_num != second_pli.Size();
             --i) {
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

        for (std::size_t i = 0; i != thresholds.size() && start_cluster_num != second_pli.Size();
             ++i) {
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

public:
    CrossISNBuilder(std::shared_ptr<ISNInfo> isn_info,
                    std::shared_ptr<DistanceCalculator> distance_calculator,
                    PliShard const& first_pli_shard, PliShard const& second_pli_shard,
                    std::vector<std::vector<Bitset>> const& offset_to_predicates,
                    std::size_t bitset_size)
        : isn_info_(isn_info),
          distance_calculator_(distance_calculator),
          first_pli_shard_(first_pli_shard),
          second_pli_shard_(second_pli_shard),
          offset_to_predicates_(offset_to_predicates),
          bitset_size_(bitset_size) {}

    boost::unordered::unordered_flat_set<ClueType> BuildISNs() {
        std::size_t num_tuple_pairs = first_pli_shard_.Range() * second_pli_shard_.Range();
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
