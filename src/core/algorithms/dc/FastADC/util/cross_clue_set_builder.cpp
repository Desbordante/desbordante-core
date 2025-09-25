#include "dc/FastADC/util/cross_clue_set_builder.h"

#include <bitset>
#include <cstdint>

#include "dc/FastADC/model/pli_shard.h"
#include "dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

CrossClueSetBuilder::CrossClueSetBuilder(PliShard const& shard1, PliShard const& shard2)
    : plis1_(shard1.Plis()),
      plis2_(shard2.Plis()),
      evidence_count_(shard1.Range() * shard2.Range()) {}

void CrossClueSetBuilder::BuildClueSet(PredicatePacks const& packs,
                                       std::vector<Clue>& forward_clues,
                                       std::vector<Clue>& reverse_clues, ClueSet& clue_set) {
    forward_clues.assign(evidence_count_, Clue());
    reverse_clues.assign(evidence_count_, Clue());

    if (forward_clues.size() < evidence_count_) forward_clues.resize(evidence_count_, Clue());
    if (reverse_clues.size() < evidence_count_) reverse_clues.resize(evidence_count_, Clue());

    for (auto const& cat_pack : packs.str_single) {
        CorrectStrSingle(forward_clues, reverse_clues, plis1_[cat_pack.left_idx],
                         plis2_[cat_pack.right_idx], cat_pack.eq_mask);
    }

    for (auto const& cat_pack : packs.str_cross) {
        CorrectStrCross(forward_clues, plis1_[cat_pack.left_idx], plis2_[cat_pack.right_idx],
                        cat_pack.eq_mask);
        CorrectStrCross(reverse_clues, plis2_[cat_pack.left_idx], plis1_[cat_pack.right_idx],
                        cat_pack.eq_mask);
    }

    for (auto const& num_pack : packs.num_single) {
        CorrectNumSingle(forward_clues, reverse_clues, plis1_[num_pack.left_idx],
                         plis2_[num_pack.right_idx], num_pack.eq_mask, num_pack.gt_mask);
    }

    for (auto const& num_pack : packs.num_cross) {
        CorrectNumCross(forward_clues, plis1_[num_pack.left_idx], plis2_[num_pack.right_idx],
                        num_pack.eq_mask, num_pack.gt_mask);
        CorrectNumCross(reverse_clues, plis2_[num_pack.left_idx], plis1_[num_pack.right_idx],
                        num_pack.eq_mask, num_pack.gt_mask);
    }

    AccumulateClues(clue_set, forward_clues, reverse_clues);
}

void CrossClueSetBuilder::SetSingleEQ(std::vector<Clue>& clues1, std::vector<Clue>& clues2,
                                      Pli const& pli1, size_t i, Pli const& pli2, size_t j,
                                      Clue const& mask) {
    size_t beg1 = pli1.pli_shard_->Beg();
    size_t beg2 = pli2.pli_shard_->Beg();
    size_t range1 = pli1.pli_shard_->Range();
    size_t range2 = pli2.pli_shard_->Range();

    for (size_t tid1 : pli1.Get(i)) {
        int64_t t1 = tid1 - beg1, r1 = t1 * range2 - beg2;
        for (size_t tid2 : pli2.Get(j)) {
            clues1[r1 + tid2] |= mask;
            clues2[(tid2 - beg2) * range1 + t1] |= mask;
        }
    }
}

void CrossClueSetBuilder::CorrectStrSingle(std::vector<Clue>& clues1, std::vector<Clue>& clues2,
                                           Pli const& pivotPli, Pli const& probePli,
                                           Clue const& mask) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();

    for (size_t i = 0; i < pivot_keys.size(); ++i) {
        size_t j;
        if (probePli.TryGetClusterIdByKey(pivot_keys[i], j)) {
            SetSingleEQ(clues1, clues2, pivotPli, i, probePli, j, mask);
        }
    }
}

void CrossClueSetBuilder::SetCrossEQ(std::vector<Clue>& clues, Pli const& pli1, size_t i,
                                     Pli const& pli2, size_t j, Clue const& mask) {
    size_t tid_beg1 = pli1.pli_shard_->Beg();
    size_t tid_beg2 = pli2.pli_shard_->Beg();
    size_t tid_range2 = pli2.pli_shard_->Range();

    for (size_t tid1 : pli1.Get(i)) {
        int64_t r1 = (tid1 - tid_beg1) * tid_range2 - tid_beg2;
        for (size_t tid2 : pli2.Get(j)) {
            clues[r1 + tid2] |= mask;
        }
    }
}

void CrossClueSetBuilder::CorrectStrCross(std::vector<Clue>& clues, Pli const& pivotPli,
                                          Pli const& probePli, Clue const& mask) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();

    for (size_t i = 0; i < pivot_keys.size(); ++i) {
        size_t j;
        if (probePli.TryGetClusterIdByKey(pivot_keys[i], j)) {
            SetCrossEQ(clues, pivotPli, i, probePli, j, mask);
        }
    }
}

void CrossClueSetBuilder::SetReverseGT(std::vector<Clue>& reverseArray, Pli const& probePli,
                                       size_t to, Pli const& pivotPli, size_t i, Clue const& mask) {
    size_t probe_beg = probePli.pli_shard_->Beg();
    size_t pivot_beg = pivotPli.pli_shard_->Beg();
    size_t pivot_range = pivotPli.pli_shard_->Range();

    for (size_t j = 0; j < to; ++j) {
        for (size_t probe_tid : probePli.Get(j)) {
            int64_t r2 = (probe_tid - probe_beg) * pivot_range - pivot_beg;
            for (size_t pivot_tid : pivotPli.Get(i)) {
                reverseArray[r2 + pivot_tid] |= mask;
            }
        }
    }
}

void CrossClueSetBuilder::SetForwardGT(std::vector<Clue>& forwardArray, Pli const& pivotPli,
                                       size_t i, Pli const& probePli, size_t from,
                                       Clue const& mask) {
    size_t pivot_beg = pivotPli.pli_shard_->Beg();
    size_t probe_beg = probePli.pli_shard_->Beg();
    size_t probe_range = probePli.pli_shard_->Range();

    for (size_t pivot_tid : pivotPli.Get(i)) {
        int64_t r1 = (pivot_tid - pivot_beg) * probe_range - probe_beg;
        for (size_t j = from; j < probePli.Size(); ++j) {
            for (size_t probe_tid : probePli.Get(j)) {
                forwardArray[r1 + probe_tid] |= mask;
            }
        }
    }
}

void CrossClueSetBuilder::CorrectNumSingle(std::vector<Clue>& forwardArray,
                                           std::vector<Clue>& reverseArray, Pli const& pivotPli,
                                           Pli const& probePli, Clue const& eqMask,
                                           Clue const& gtMask) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();
    std::vector<size_t> const& probe_keys = probePli.GetKeys();

    for (size_t i = 0, j = 0; i < pivot_keys.size(); ++i) {
        size_t pivot_key = pivot_keys[i];
        j = probePli.GetFirstIndexWhereKeyIsLTE(pivot_key, j);  // current position in probePli
        size_t reverse_to = j;

        if (j == probe_keys.size()) {  // pivotKey is less than all probeKeys
            for (size_t ii = i; ii < pivot_keys.size(); ++ii) {
                SetReverseGT(reverseArray, probePli, j, pivotPli, ii, gtMask);
            }
            break;
        } else if (pivot_key == probe_keys[j]) {
            SetSingleEQ(forwardArray, reverseArray, pivotPli, i, probePli, j, eqMask);
            j++;
        }

        SetForwardGT(forwardArray, pivotPli, i, probePli, j, gtMask);
        SetReverseGT(reverseArray, probePli, reverse_to, pivotPli, i, gtMask);
    }
}

void CrossClueSetBuilder::CorrectNumCross(std::vector<Clue>& forwardArray, Pli const& pivotPli,
                                          Pli const& probePli, Clue const& eqMask,
                                          Clue const& gtMask) {
    std::vector<size_t> const& pivot_keys = pivotPli.GetKeys();
    std::vector<size_t> const& probe_keys = probePli.GetKeys();

    for (size_t i = 0, j = 0; i < pivot_keys.size(); ++i) {
        size_t key = pivot_keys[i];
        j = probePli.GetFirstIndexWhereKeyIsLTE(key, j);  // current position in probePli

        if (j == probe_keys.size()) {
            break;
        }

        if (key == probe_keys[j]) {
            SetCrossEQ(forwardArray, pivotPli, i, probePli, j, eqMask);
            j++;
        }

        SetForwardGT(forwardArray, pivotPli, i, probePli, j, gtMask);
    }
}

}  // namespace algos::fastadc
