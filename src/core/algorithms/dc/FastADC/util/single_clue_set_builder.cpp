#include "single_clue_set_builder.h"

#include <stdexcept>
#include <stdint.h>

#include "dc/FastADC/model/pli_shard.h"
#include "dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

SingleClueSetBuilder::SingleClueSetBuilder(PliShard const& shard)
    : plis_(shard.plis),
      tid_beg_(shard.beg),
      tid_range_(shard.Range()),
      evidence_count_(tid_range_ * tid_range_) {}

void SingleClueSetBuilder::BuildClueSet(PredicatePacks const& packs, std::vector<Clue>& clues,
                                        ClueSet& clue_set) {
    clues.assign(evidence_count_, Clue());

    if (clues.size() < evidence_count_) clues.resize(evidence_count_, Clue());

    for (auto const& cat_pack : packs.str_single) {
        CorrectStrSingle(clues, plis_[cat_pack.left_idx], cat_pack.eq_mask);
    }

    for (auto const& cat_pack : packs.str_cross) {
        CorrectStrCross(clues, plis_[cat_pack.left_idx], plis_[cat_pack.right_idx],
                        cat_pack.eq_mask);
    }

    for (auto const& num_pack : packs.num_single) {
        CorrectNumSingle(clues, plis_[num_pack.left_idx], num_pack.eq_mask, num_pack.gt_mask);
    }

    for (auto const& num_pack : packs.num_cross) {
        CorrectNumCross(clues, plis_[num_pack.left_idx], plis_[num_pack.right_idx],
                        num_pack.eq_mask, num_pack.gt_mask);
    }

    AccumulateClues(clue_set, clues);

    // Reflex evidence check and removal:
    Clue reflex_clue{};  // All bits zero
    clue_set[reflex_clue] -= tid_range_;
    if (clue_set[reflex_clue] == 0) clue_set.erase(clue_set.find(reflex_clue));
}

void SingleClueSetBuilder::SetSingleEQ(std::vector<Clue>& clues, Pli::Cluster const& cluster,
                                       Clue const& mask) {
    for (size_t i = 0; i < cluster.size() - 1; ++i) {
        int64_t t1 = cluster[i] - tid_beg_;
        int64_t r1 = t1 * tid_range_;
        for (size_t j = i + 1; j < cluster.size(); ++j) {
            int64_t t2 = cluster[j] - tid_beg_;
            clues[r1 + t2] |= mask;
            clues[t2 * tid_range_ + t1] |= mask;
        }
    }
}

void SingleClueSetBuilder::CorrectStrSingle(std::vector<Clue>& clues, Pli const& pli,
                                            Clue const& mask) {
    for (size_t i = 0; i < pli.Size(); ++i) {
        if (pli.Get(i).size() > 1) {
            SetSingleEQ(clues, pli.Get(i), mask);
        }
    }
}

void SingleClueSetBuilder::SetCrossEQ(std::vector<Clue>& clues, Pli::Cluster const& pivotCluster,
                                      Pli::Cluster const& probeCluster, Clue const& mask) {
    for (size_t tid1 : pivotCluster) {
        int64_t r1 = (tid1 - tid_beg_) * tid_range_ - tid_beg_;
        for (size_t tid2 : probeCluster) {
            if (tid1 != tid2) {
                clues[r1 + tid2] |= mask;
            }
        }
    }
}

void SingleClueSetBuilder::CorrectStrCross(std::vector<Clue>& clues, Pli const& pivotPli,
                                           Pli const& probePli, Clue const& mask) {
    auto const& pivot_clusters = pivotPli.GetClusters();
    auto const& probe_clusters = probePli.GetClusters();
    auto const& pivot_keys = pivotPli.GetKeys();

    for (size_t i = 0; i < pivot_keys.size(); ++i) {
        try {
            size_t j = probePli.GetClusterIdByKey(pivot_keys[i]);
            SetCrossEQ(clues, pivot_clusters[i], probe_clusters[j], mask);
        } catch (std::runtime_error const&) {
            // Ignore and continue if no cluster is found by key
        }
    }
}

void SingleClueSetBuilder::SetGT(std::vector<Clue>& clues, Pli::Cluster const& pivotCluster,
                                 Pli const& probePli, size_t from, Clue const& mask) {
    for (size_t pivot_tid : pivotCluster) {
        int64_t r1 = (pivot_tid - tid_beg_) * tid_range_ - tid_beg_;
        for (size_t j = from; j < probePli.Size(); ++j) {
            for (size_t probe_tid : probePli.Get(j)) {
                if (pivot_tid != probe_tid) {
                    clues[r1 + probe_tid] |= mask;
                }
            }
        }
    }
}

void SingleClueSetBuilder::CorrectNumSingle(std::vector<Clue>& clues, Pli const& pli,
                                            Clue const& eqMask, Clue const& gtMask) {
    for (size_t i = 0; i < pli.Size(); ++i) {
        auto const& cluster = pli.Get(i);
        if (cluster.size() > 1) {
            SetSingleEQ(clues, cluster, eqMask);
        }
        if (i < pli.Size() - 1) {
            SetGT(clues, cluster, pli, i + 1, gtMask);
        }
    }
}

void SingleClueSetBuilder::CorrectNumCross(std::vector<Clue>& clues, Pli const& pivotPli,
                                           Pli const& probePli, Clue const& eqMask,
                                           Clue const& gtMask) {
    auto const& pivot_keys = pivotPli.GetKeys();
    auto const& probe_keys = probePli.GetKeys();

    for (size_t i = 0, j = 0; i < pivot_keys.size(); ++i) {
        size_t key = pivot_keys[i];

        j = probePli.GetFirstIndexWhereKeyIsLTE(key, j);

        if (j == probe_keys.size()) break;

        if (key == probe_keys[j]) {
            SetCrossEQ(clues, pivotPli.Get(i), probePli.Get(j), eqMask);
            j++;
        }

        SetGT(clues, pivotPli.Get(i), probePli, j, gtMask);
    }
}

}  // namespace algos::fastadc
