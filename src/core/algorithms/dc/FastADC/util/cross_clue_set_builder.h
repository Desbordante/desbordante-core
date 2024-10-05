#pragma once

#include <vector>

#include "../model/pli_shard.h"
#include "evidence_aux_structures_builder.h"

namespace algos::fastadc {

class CrossClueSetBuilder {
public:
    CrossClueSetBuilder(PliShard const& shard1, PliShard const& shard2);
    CrossClueSetBuilder(CrossClueSetBuilder const& other) = delete;
    CrossClueSetBuilder& operator=(CrossClueSetBuilder const& other) = delete;
    CrossClueSetBuilder(CrossClueSetBuilder&& other) noexcept = default;
    CrossClueSetBuilder& operator=(CrossClueSetBuilder&& other) noexcept = delete;

    ClueSet BuildClueSet(PredicatePacks const& packs);

private:
    std::vector<Pli> const& plis1_;
    std::vector<Pli> const& plis2_;
    size_t evidence_count_;

    void SetSingleEQ(std::vector<Clue>& clues1, std::vector<Clue>& clues2, Pli const& pli1,
                     size_t i, Pli const& pli2, size_t j, Clue const& mask);
    void CorrectStrSingle(std::vector<Clue>& clues1, std::vector<Clue>& clues2, Pli const& pivotPli,
                          Pli const& probePli, Clue const& mask);
    void SetCrossEQ(std::vector<Clue>& clues, Pli const& pli1, size_t i, Pli const& pli2, size_t j,
                    Clue const& mask);
    void CorrectStrCross(std::vector<Clue>& clues, Pli const& pivotPli, Pli const& probePli,
                         Clue const& mask);
    void SetReverseGT(std::vector<Clue>& reverseArray, Pli const& probePli, size_t to,
                      Pli const& pivotPli, size_t i, Clue const& mask);
    void SetForwardGT(std::vector<Clue>& forwardArray, Pli const& pivotPli, size_t i,
                      Pli const& probePli, size_t from, Clue const& mask);
    void CorrectNumSingle(std::vector<Clue>& forwardArray, std::vector<Clue>& reverseArray,
                          Pli const& pivotPli, Pli const& probePli, Clue const& eqMask,
                          Clue const& gtMask);
    void CorrectNumCross(std::vector<Clue>& forwardArray, Pli const& pivotPli, Pli const& probePli,
                         Clue const& eqMask, Clue const& gtMask);
};

}  // namespace algos::fastadc
