#pragma once

#include "dc/FastADC/model/pli_shard.h"
#include "dc/FastADC/util/common_clue_set_builder.h"
#include "dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);

}  // namespace algos::fastadc
