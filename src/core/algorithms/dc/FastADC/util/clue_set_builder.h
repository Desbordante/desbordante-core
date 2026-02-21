#pragma once

#include "core/algorithms/dc/FastADC/model/pli_shard.h"
#include "core/algorithms/dc/FastADC/util/common_clue_set_builder.h"
#include "core/algorithms/dc/FastADC/util/evidence_aux_structures_builder.h"
#include "core/util/worker_thread_pool.h"

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);

ClueSet BuildClueSetParallel(std::vector<PliShard> const& pliShards, PredicatePacks const& packs,
                             util::WorkerThreadPool* thread_pool);

}  // namespace algos::fastadc
