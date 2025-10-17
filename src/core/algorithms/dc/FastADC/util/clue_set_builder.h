#pragma once

#include <vector>

#include <easylogging++.h>

#include "dc/FastADC/model/pli_shard.h"
#include "dc/FastADC/util/common_clue_set_builder.h"
#include "dc/FastADC/util/evidence_aux_structures_builder.h"

namespace algos {
namespace fastadc {
class PliShard;
struct PredicatePacks;
}  // namespace fastadc
}  // namespace algos

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);

}  // namespace algos::fastadc
