#pragma once

#include <vector>

#include "dc/common_clue_set_builder.h"
#include "dc/pli_shard.h"

namespace algos::fastadc {

class CrossClueSetBuilder : public CommonClueSetBuilder {
public:
    CrossClueSetBuilder(PredicateBuilder const& pbuilder, PliShard const& shard1,
                        PliShard const& shard2);

    ClueSet BuildClueSet() override;

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
