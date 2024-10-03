#pragma once

#include <easylogging++.h>

#include "dc/clue_set_builder.h"
#include "dc/evidence_set.h"

namespace algos::fastadc {

/**
 * Creates EvidenceSet, which is just a vector of evidences with some extra methods
 */
class EvidenceSetBuilder {
public:
    EvidenceSetBuilder(PredicateBuilder const& pbuilder, std::vector<PliShard> const& pli_shards) {
        ClueSetBuilder cluebuilder(pbuilder);

        clue_set_ = cluebuilder.BuildClueSet(pli_shards);
        correction_map_ = CommonClueSetBuilder::GetCorrectionMap();
        BuildCardinalityMask(pbuilder);
    }

    void BuildEvidenceSet() {
        evidence_set_.Reserve(clue_set_.size());

        for (auto const& [clue, count] : clue_set_) {
            evidence_set_.EmplaceBack(clue, count, cardinality_mask_, correction_map_);
        }

        LOG(DEBUG) << " [Evidence] # of evidences: " << evidence_set_.Size();
        LOG(DEBUG) << " [Evidence] Accumulated evidence count: " << evidence_set_.GetTotalCount();
    }

    // For tests
    PredicateBitset const& GetCardinalityMask() const {
        return cardinality_mask_;
    }

    EvidenceSet&& GetEvidenceSet() {
        return std::move(evidence_set_);
    }

private:
    void BuildCardinalityMask(PredicateBuilder const& pbuilder);

    ClueSet clue_set_;
    std::vector<PredicateBitset> correction_map_;
    PredicateBitset cardinality_mask_;
    EvidenceSet evidence_set_;
};

}  // namespace algos::fastadc
