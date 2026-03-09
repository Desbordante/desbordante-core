#include "core/algorithms/dc/FastADC/util/cross_clue_set_builder.h"

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
CrossClueSetBuilderT<ClueT>::CrossClueSetBuilderT(PliShard const& shard1, PliShard const& shard2)
    : plis1_(shard1.Plis()),
      plis2_(shard2.Plis()),
      evidence_count_(shard1.Range() * shard2.Range()) {}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::BuildClueSet(PredicatePacks const& packs,
                                              std::vector<ClueT>& forward_clues,
                                              std::vector<ClueT>& reverse_clues,
                                              ClueSetT<ClueT>& clue_set) {
    forward_clues.assign(evidence_count_, ClueT{});
    reverse_clues.assign(evidence_count_, ClueT{});

    for (auto const& cat_pack : packs.str_single) {
        CorrectStrSingle(forward_clues, reverse_clues, plis1_[cat_pack.left_idx],
                         plis2_[cat_pack.right_idx], cat_pack.eq_pos);
    }

    for (auto const& cat_pack : packs.str_cross) {
        CorrectStrCross(forward_clues, plis1_[cat_pack.left_idx], plis2_[cat_pack.right_idx],
                        cat_pack.eq_pos);
        CorrectStrCross(reverse_clues, plis2_[cat_pack.left_idx], plis1_[cat_pack.right_idx],
                        cat_pack.eq_pos);
    }

    for (auto const& num_pack : packs.num_single) {
        CorrectNumSingle(forward_clues, reverse_clues, plis1_[num_pack.left_idx],
                         plis2_[num_pack.right_idx], num_pack.eq_pos, num_pack.gt_pos);
    }

    for (auto const& num_pack : packs.num_cross) {
        CorrectNumCross(forward_clues, plis1_[num_pack.left_idx], plis2_[num_pack.right_idx],
                        num_pack.eq_pos, num_pack.gt_pos);
        CorrectNumCross(reverse_clues, plis2_[num_pack.left_idx], plis1_[num_pack.right_idx],
                        num_pack.eq_pos, num_pack.gt_pos);
    }

    AccumulateClues(clue_set, forward_clues, reverse_clues);
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::SetSingleEQ(std::vector<ClueT>& clues1,
                                             std::vector<ClueT>& clues2, Pli const& pli1,
                                             size_t i, Pli const& pli2, size_t j,
                                             size_t mask_pos) {
    size_t beg1 = pli1.pli_shard_->Beg();
    size_t beg2 = pli2.pli_shard_->Beg();
    size_t range1 = pli1.pli_shard_->Range();
    size_t range2 = pli2.pli_shard_->Range();

    for (size_t tid1 : pli1.Get(i)) {
        int64_t t1 = tid1 - beg1;
        int64_t r1 = t1 * range2 - beg2;
        for (size_t tid2 : pli2.Get(j)) {
            SetClueBit(clues1[r1 + tid2], mask_pos);
            SetClueBit(clues2[(tid2 - beg2) * range1 + t1], mask_pos);
        }
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::CorrectStrSingle(std::vector<ClueT>& clues1,
                                                  std::vector<ClueT>& clues2,
                                                  Pli const& pivotPli,
                                                  Pli const& probePli,
                                                  size_t mask_pos) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();

    for (size_t i = 0; i < pivot_keys.size(); ++i) {
        size_t j;
        if (probePli.TryGetClusterIdByKey(pivot_keys[i], j)) {
            SetSingleEQ(clues1, clues2, pivotPli, i, probePli, j, mask_pos);
        }
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::SetCrossEQ(std::vector<ClueT>& clues, Pli const& pli1,
                                            size_t i, Pli const& pli2, size_t j,
                                            size_t mask_pos) {
    size_t tid_beg1 = pli1.pli_shard_->Beg();
    size_t tid_beg2 = pli2.pli_shard_->Beg();
    size_t tid_range2 = pli2.pli_shard_->Range();

    for (size_t tid1 : pli1.Get(i)) {
        int64_t r1 = (tid1 - tid_beg1) * tid_range2 - tid_beg2;
        for (size_t tid2 : pli2.Get(j)) {
            SetClueBit(clues[r1 + tid2], mask_pos);
        }
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::CorrectStrCross(std::vector<ClueT>& clues, Pli const& pivotPli,
                                                 Pli const& probePli, size_t mask_pos) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();

    for (size_t i = 0; i < pivot_keys.size(); ++i) {
        size_t j;
        if (probePli.TryGetClusterIdByKey(pivot_keys[i], j)) {
            SetCrossEQ(clues, pivotPli, i, probePli, j, mask_pos);
        }
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::SetReverseGT(std::vector<ClueT>& reverseArray,
                                              Pli const& probePli, size_t to,
                                              Pli const& pivotPli, size_t i,
                                              size_t mask_pos) {
    size_t probe_beg = probePli.pli_shard_->Beg();
    size_t pivot_beg = pivotPli.pli_shard_->Beg();
    size_t pivot_range = pivotPli.pli_shard_->Range();

    for (size_t j = 0; j < to; ++j) {
        for (size_t probe_tid : probePli.Get(j)) {
            int64_t r2 = (probe_tid - probe_beg) * pivot_range - pivot_beg;
            for (size_t pivot_tid : pivotPli.Get(i)) {
                SetClueBit(reverseArray[r2 + pivot_tid], mask_pos);
            }
        }
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::SetForwardGT(std::vector<ClueT>& forwardArray,
                                              Pli const& pivotPli, size_t i,
                                              Pli const& probePli, size_t from,
                                              size_t mask_pos) {
    size_t pivot_beg = pivotPli.pli_shard_->Beg();
    size_t probe_beg = probePli.pli_shard_->Beg();
    size_t probe_range = probePli.pli_shard_->Range();

    for (size_t pivot_tid : pivotPli.Get(i)) {
        int64_t r1 = (pivot_tid - pivot_beg) * probe_range - probe_beg;
        for (size_t j = from; j < probePli.Size(); ++j) {
            for (size_t probe_tid : probePli.Get(j)) {
                SetClueBit(forwardArray[r1 + probe_tid], mask_pos);
            }
        }
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::CorrectNumSingle(std::vector<ClueT>& forwardArray,
                                                  std::vector<ClueT>& reverseArray,
                                                  Pli const& pivotPli, Pli const& probePli,
                                                  size_t eq_pos, size_t gt_pos) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();
    std::vector<size_t> const& probe_keys = probePli.GetKeys();

    for (size_t i = 0, j = 0; i < pivot_keys.size(); ++i) {
        size_t pivot_key = pivot_keys[i];
        j = probePli.GetFirstIndexWhereKeyIsLTE(pivot_key, j);
        size_t reverse_to = j;

        if (j == probe_keys.size()) {
            for (size_t ii = i; ii < pivot_keys.size(); ++ii) {
                SetReverseGT(reverseArray, probePli, j, pivotPli, ii, gt_pos);
            }
            break;
        }

        if (pivot_key == probe_keys[j]) {
            SetSingleEQ(forwardArray, reverseArray, pivotPli, i, probePli, j, eq_pos);
            ++j;
        }

        SetForwardGT(forwardArray, pivotPli, i, probePli, j, gt_pos);
        SetReverseGT(reverseArray, probePli, reverse_to, pivotPli, i, gt_pos);
    }
}

template <typename ClueT>
void CrossClueSetBuilderT<ClueT>::CorrectNumCross(std::vector<ClueT>& forwardArray,
                                                 Pli const& pivotPli,
                                                 Pli const& probePli, size_t eq_pos,
                                                 size_t gt_pos) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();
    std::vector<size_t> const& probe_keys = probePli.GetKeys();

    for (size_t i = 0, j = 0; i < pivot_keys.size(); ++i) {
        size_t key = pivot_keys[i];
        j = probePli.GetFirstIndexWhereKeyIsLTE(key, j);

        if (j == probe_keys.size()) {
            break;
        }

        if (key == probe_keys[j]) {
            SetCrossEQ(forwardArray, pivotPli, i, probePli, j, eq_pos);
            ++j;
        }

        SetForwardGT(forwardArray, pivotPli, i, probePli, j, gt_pos);
    }
}

template class CrossClueSetBuilderT<uint8_t>;
template class CrossClueSetBuilderT<uint16_t>;
template class CrossClueSetBuilderT<uint32_t>;
template class CrossClueSetBuilderT<uint64_t>;
template class CrossClueSetBuilderT<Clue>;

}  // namespace algos::fastadc
