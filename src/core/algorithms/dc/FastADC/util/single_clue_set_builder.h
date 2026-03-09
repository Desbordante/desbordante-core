#pragma once

#include <cstddef>
#include <vector>

#include "core/algorithms/dc/FastADC/model/pli_shard.h"
#include "core/algorithms/dc/FastADC/util/common_clue_set_builder.h"
#include "core/algorithms/dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

template <typename ClueT>
class SingleClueSetBuilderT {
public:
    explicit SingleClueSetBuilderT(PliShard const& shard);

    SingleClueSetBuilderT(SingleClueSetBuilderT const& other) = delete;
    SingleClueSetBuilderT& operator=(SingleClueSetBuilderT const& other) = delete;
    SingleClueSetBuilderT(SingleClueSetBuilderT&& other) noexcept = default;
    SingleClueSetBuilderT& operator=(SingleClueSetBuilderT&& other) noexcept = delete;

    void BuildClueSet(PredicatePacks const& packs, std::vector<ClueT>& clues,
                     ClueSetT<ClueT>& clue_set);

private:
    std::vector<Pli> const& plis_;
    size_t tid_beg_;
    size_t tid_range_;
    size_t evidence_count_;

    void SetSingleEQ(std::vector<ClueT>& clues, Pli::Cluster const& cluster, size_t mask_pos);
    void CorrectStrSingle(std::vector<ClueT>& clues, Pli const& pli, size_t mask_pos);

    void SetCrossEQ(std::vector<ClueT>& clues, Pli::Cluster const& pivotCluster,
                   Pli::Cluster const& probeCluster, size_t mask_pos);
    void CorrectStrCross(std::vector<ClueT>& clues, Pli const& pivotPli, Pli const& probePli,
                        size_t mask_pos);

    void SetGT(std::vector<ClueT>& clues, Pli::Cluster const& pivotCluster, Pli const& probePli,
              size_t from, size_t mask_pos);

    void CorrectNumSingle(std::vector<ClueT>& clues, Pli const& pli, size_t eq_pos,
                         size_t gt_pos);
    void CorrectNumCross(std::vector<ClueT>& clues, Pli const& pivotPli, Pli const& probePli,
                        size_t eq_pos, size_t gt_pos);
};

using SingleClueSetBuilder = SingleClueSetBuilderT<Clue>;

}  // namespace algos::fastadc
