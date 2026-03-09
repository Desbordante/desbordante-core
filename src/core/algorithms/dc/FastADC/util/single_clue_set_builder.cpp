#include "core/algorithms/dc/FastADC/util/single_clue_set_builder.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/algorithms/dc/FastADC/model/pli_shard.h"
#include "core/algorithms/dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

template <typename ClueT>
inline void SetClueBit(ClueT& clue, size_t pos) {
    if constexpr (std::is_integral_v<ClueT>) {
        clue |= static_cast<ClueT>(uint64_t{1} << pos);
    } else {
        clue.set(pos);
    }
}

template <typename ClueT>
SingleClueSetBuilderT<ClueT>::SingleClueSetBuilderT(PliShard const& shard)
    : plis_(shard.Plis()),
      tid_beg_(shard.Beg()),
      tid_range_(shard.Range()),
      evidence_count_(tid_range_ * tid_range_) {}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::BuildClueSet(PredicatePacks const& packs,
                                               std::vector<ClueT>& clues,
                                               ClueSetT<ClueT>& clue_set) {
    clues.assign(evidence_count_, ClueT{});

    for (auto const& cat_pack : packs.str_single) {
        CorrectStrSingle(clues, plis_[cat_pack.left_idx], cat_pack.eq_pos);
    }

    for (auto const& cat_pack : packs.str_cross) {
        CorrectStrCross(clues, plis_[cat_pack.left_idx], plis_[cat_pack.right_idx],
                        cat_pack.eq_pos);
    }

    for (auto const& num_pack : packs.num_single) {
        CorrectNumSingle(clues, plis_[num_pack.left_idx], num_pack.eq_pos, num_pack.gt_pos);
    }

    for (auto const& num_pack : packs.num_cross) {
        CorrectNumCross(clues, plis_[num_pack.left_idx], plis_[num_pack.right_idx],
                        num_pack.eq_pos, num_pack.gt_pos);
    }

    AccumulateClues(clue_set, clues);

    ClueT reflex_clue{};
    clue_set[reflex_clue] -= tid_range_;
    if (clue_set[reflex_clue] == 0) {
        clue_set.erase(clue_set.find(reflex_clue));
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::SetSingleEQ(std::vector<ClueT>& clues,
                                              Pli::Cluster const& cluster,
                                              size_t mask_pos) {
    for (size_t i = 0; i + 1 < cluster.size(); ++i) {
        int64_t t1 = cluster[i] - tid_beg_;
        int64_t r1 = t1 * tid_range_;
        for (size_t j = i + 1; j < cluster.size(); ++j) {
            int64_t t2 = cluster[j] - tid_beg_;
            SetClueBit(clues[r1 + t2], mask_pos);
            SetClueBit(clues[t2 * tid_range_ + t1], mask_pos);
        }
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::CorrectStrSingle(std::vector<ClueT>& clues, Pli const& pli,
                                                   size_t mask_pos) {
    for (size_t i = 0; i < pli.Size(); ++i) {
        if (pli.Get(i).size() > 1) {
            SetSingleEQ(clues, pli.Get(i), mask_pos);
        }
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::SetCrossEQ(std::vector<ClueT>& clues,
                                             Pli::Cluster const& pivotCluster,
                                             Pli::Cluster const& probeCluster,
                                             size_t mask_pos) {
    for (size_t tid1 : pivotCluster) {
        int64_t r1 = (tid1 - tid_beg_) * tid_range_ - tid_beg_;
        for (size_t tid2 : probeCluster) {
            if (tid1 != tid2) {
                SetClueBit(clues[r1 + tid2], mask_pos);
            }
        }
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::CorrectStrCross(std::vector<ClueT>& clues, Pli const& pivotPli,
                                                  Pli const& probePli,
                                                  size_t mask_pos) {
    std::vector<Pli::Cluster> const& pivot_clusters = pivotPli.GetClusters();
    std::vector<Pli::Cluster> const& probe_clusters = probePli.GetClusters();
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();

    for (size_t i = 0; i < pivot_keys.size(); ++i) {
        size_t j;
        if (probePli.TryGetClusterIdByKey(pivot_keys[i], j)) {
            SetCrossEQ(clues, pivot_clusters[i], probe_clusters[j], mask_pos);
        }
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::SetGT(std::vector<ClueT>& clues,
                                        Pli::Cluster const& pivotCluster, Pli const& probePli,
                                        size_t from, size_t mask_pos) {
    for (size_t pivot_tid : pivotCluster) {
        int64_t r1 = (pivot_tid - tid_beg_) * tid_range_ - tid_beg_;
        for (size_t j = from; j < probePli.Size(); ++j) {
            for (size_t probe_tid : probePli.Get(j)) {
                if (pivot_tid != probe_tid) {
                    SetClueBit(clues[r1 + probe_tid], mask_pos);
                }
            }
        }
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::CorrectNumSingle(std::vector<ClueT>& clues, Pli const& pli,
                                                   size_t eq_pos, size_t gt_pos) {
    for (size_t i = 0; i < pli.Size(); ++i) {
        Pli::Cluster const& cluster = pli.Get(i);
        if (cluster.size() > 1) {
            SetSingleEQ(clues, cluster, eq_pos);
        }
        if (i + 1 < pli.Size()) {
            SetGT(clues, cluster, pli, i + 1, gt_pos);
        }
    }
}

template <typename ClueT>
void SingleClueSetBuilderT<ClueT>::CorrectNumCross(std::vector<ClueT>& clues, Pli const& pivotPli,
                                                  Pli const& probePli,
                                                  size_t eq_pos, size_t gt_pos) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();
    std::vector<size_t> const& probe_keys = probePli.GetKeys();

    for (size_t i = 0, j = 0; i < pivot_keys.size(); ++i) {
        size_t key = pivot_keys[i];
        j = probePli.GetFirstIndexWhereKeyIsLTE(key, j);

        if (j == probe_keys.size()) {
            break;
        }

        if (key == probe_keys[j]) {
            SetCrossEQ(clues, pivotPli.Get(i), probePli.Get(j), eq_pos);
            ++j;
        }

        SetGT(clues, pivotPli.Get(i), probePli, j, gt_pos);
    }
}

template class SingleClueSetBuilderT<uint8_t>;
template class SingleClueSetBuilderT<uint16_t>;
template class SingleClueSetBuilderT<uint32_t>;
template class SingleClueSetBuilderT<uint64_t>;
template class SingleClueSetBuilderT<Clue>;

}  // namespace algos::fastadc
