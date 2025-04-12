#pragma once

#include "core/algorithms/dc/fast_adc/model/pli_shard.h"
#include "core/algorithms/dc/fast_adc/util/common_clue_set_builder.h"
#include "core/algorithms/dc/fast_adc/util/evidence_aux_structures_builder.h"

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);

}  // namespace algos::fastadc
