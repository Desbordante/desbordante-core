#pragma once

#include <easylogging++.h>

#include "dc/FastADC/model/evidence_set.h"
#include "dc/FastADC/util/clue_set_builder.h"

namespace algos::fastadc {

/**
 * Creates EvidenceSet, which is just a vector of evidences with some extra methods
 */
class EvidenceSetBuilder {
public:
    EvidenceSet evidence_set;

    EvidenceSetBuilder(std::vector<PliShard> const& pli_shards, PredicatePacks const& packs) {
        clue_set_ = BuildClueSet(pli_shards, packs);
    }

    EvidenceSetBuilder(EvidenceSetBuilder const& other) = delete;
    EvidenceSetBuilder& operator=(EvidenceSetBuilder const& other) = delete;
    EvidenceSetBuilder(EvidenceSetBuilder&& other) noexcept = default;
    EvidenceSetBuilder& operator=(EvidenceSetBuilder&& other) noexcept = delete;

    void BuildEvidenceSet(std::vector<PredicateBitset> const& correction_map,
                          PredicateBitset const& cardinality_mask) {
        evidence_set.Reserve(clue_set_.size());

        for (auto const& [clue, count] : clue_set_) {
            evidence_set.EmplaceBack(clue, count, cardinality_mask, correction_map);
        }

        LOG(DEBUG) << " [Evidence] # of evidences: " << evidence_set.Size();
        LOG(DEBUG) << " [Evidence] Accumulated evidence count: " << evidence_set.GetTotalCount();
    }

private:
    ClueSet clue_set_;
};

}  // namespace algos::fastadc
