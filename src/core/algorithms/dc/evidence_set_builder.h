#pragma once

#include <easylogging++.h>

#include "dc/clue_set_builder.h"

namespace model {

/**
 * Creates evidence set
 */
class EvidenceSetBuilder {
public:
    EvidenceSetBuilder(PredicateBuilder const& pbuilder, std::vector<PliShard> const& pli_shards) {
        ClueSetBuilder cluebuilder(pbuilder);

        clue_set_ = cluebuilder.BuildClueSet(pli_shards);
        correction_map_ = CommonClueSetBuilder::GetCorrectionMap();
        BuildCardinalityMask(pbuilder);
    }

    Clue const& GetCardinalityMask() const {
        return cardinality_mask_;
    }

private:
    void BuildCardinalityMask(PredicateBuilder const& pbuilder);

    ClueSet clue_set_;
    std::vector<Clue> correction_map_;
    Clue cardinality_mask_;
};

}  // namespace model
