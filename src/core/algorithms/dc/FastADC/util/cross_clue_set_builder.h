#pragma once

#include <stddef.h>
#include <vector>

#include "core/algorithms/dc/FastADC/model/pli_shard.h"
#include "core/algorithms/dc/FastADC/util/common_clue_set_builder.h"
#include "core/algorithms/dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

template <typename ClueT>
class CrossClueSetBuilderT {
public:
    CrossClueSetBuilderT(PliShard const& shard1, PliShard const& shard2);
    CrossClueSetBuilderT(CrossClueSetBuilderT const& other) = delete;
    CrossClueSetBuilderT& operator=(CrossClueSetBuilderT const& other) = delete;
    CrossClueSetBuilderT(CrossClueSetBuilderT&& other) noexcept = default;
    CrossClueSetBuilderT& operator=(CrossClueSetBuilderT&& other) noexcept = delete;

    void BuildClueSet(PredicatePacks const& packs, std::vector<ClueT>& forward_clues,
                      std::vector<ClueT>& reverse_clues, ClueSetT<ClueT>& clue_set);

private:
    std::vector<Pli> const& plis1_;
    std::vector<Pli> const& plis2_;
    size_t evidence_count_;

    void SetSingleEQ(std::vector<ClueT>& clues1, std::vector<ClueT>& clues2, Pli const& pli1,
                     size_t i, Pli const& pli2, size_t j, size_t mask_pos);
    void CorrectStrSingle(std::vector<ClueT>& clues1, std::vector<ClueT>& clues2,
                          Pli const& pivotPli, Pli const& probePli, size_t mask_pos);
    void SetCrossEQ(std::vector<ClueT>& clues, Pli const& pli1, size_t i, Pli const& pli2, size_t j,
                    size_t mask_pos);
    void CorrectStrCross(std::vector<ClueT>& clues, Pli const& pivotPli, Pli const& probePli,
                         size_t mask_pos);
    void SetReverseGT(std::vector<ClueT>& reverseArray, Pli const& probePli, size_t to,
                      Pli const& pivotPli, size_t i, size_t mask_pos);
    void SetForwardGT(std::vector<ClueT>& forwardArray, Pli const& pivotPli, size_t i,
                      Pli const& probePli, size_t from, size_t mask_pos);
    void CorrectNumSingle(std::vector<ClueT>& forwardArray, std::vector<ClueT>& reverseArray,
                          Pli const& pivotPli, Pli const& probePli, size_t eq_pos, size_t gt_pos);
    void CorrectNumCross(std::vector<ClueT>& forwardArray, Pli const& pivotPli, Pli const& probePli,
                         size_t eq_pos, size_t gt_pos);
};

using CrossClueSetBuilder = CrossClueSetBuilderT<Clue>;

}  // namespace algos::fastadc
