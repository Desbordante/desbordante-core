#pragma once

#include <easylogging++.h>

#include "../model/evidence_set.h"
#include "clue_set_builder.h"

namespace algos::fastadc {

/**
 * Creates EvidenceSet, which is just a vector of evidences with some extra methods
 */
class EvidenceSetBuilder {
public:
    EvidenceSet evidence_set;

    EvidenceSetBuilder(PredicateBuilder const& pbuilder, std::vector<PliShard> const& pli_shards) {
        clue_set_ = ClueSetBuilder{pbuilder}.BuildClueSet(pli_shards);
        correction_map_ = CommonClueSetBuilder::GetCorrectionMap();
        BuildCardinalityMask(pbuilder);
    }

    EvidenceSetBuilder(EvidenceSetBuilder const& other) = delete;
    EvidenceSetBuilder& operator=(EvidenceSetBuilder const& other) = delete;
    EvidenceSetBuilder(EvidenceSetBuilder&& other) noexcept = default;
    EvidenceSetBuilder& operator=(EvidenceSetBuilder&& other) noexcept = default;

    void BuildEvidenceSet() {
        evidence_set.Reserve(clue_set_.size());

        for (auto const& [clue, count] : clue_set_) {
            evidence_set.EmplaceBack(clue, count, cardinality_mask_, correction_map_);
        }

        LOG(DEBUG) << " [Evidence] # of evidences: " << evidence_set.Size();
        LOG(DEBUG) << " [Evidence] Accumulated evidence count: " << evidence_set.GetTotalCount();
    }

    // For tests
    PredicateBitset const& GetCardinalityMask() const {
        return cardinality_mask_;
    }

private:
    void BuildCardinalityMask(PredicateBuilder const& pbuilder);

    ClueSet clue_set_;
    std::vector<PredicateBitset> correction_map_;
    PredicateBitset cardinality_mask_;
};

}  // namespace algos::fastadc
