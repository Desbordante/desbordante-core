#pragma once

#include <cstdint>

#include "core/algorithms/dc/FastADC/model/pli_shard.h"
#include "core/algorithms/dc/FastADC/util/common_clue_set_builder.h"
#include "core/algorithms/dc/FastADC/util/evidence_aux_structures_builder.h"
#include "core/util/worker_thread_pool.h"

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);

ClueSetT<uint8_t> BuildClueSet8(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);
ClueSetT<uint16_t> BuildClueSet16(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);
ClueSetT<uint32_t> BuildClueSet32(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);
ClueSetT<uint64_t> BuildClueSet64(std::vector<PliShard> const& pliShards, PredicatePacks const& packs);

ClueSet BuildClueSetParallel(std::vector<PliShard> const& pliShards, PredicatePacks const& packs,
                             util::WorkerThreadPool* thread_pool);

ClueSetT<uint8_t> BuildClueSetParallel8(std::vector<PliShard> const& pliShards,
                                        PredicatePacks const& packs,
                                        util::WorkerThreadPool* thread_pool);
ClueSetT<uint16_t> BuildClueSetParallel16(std::vector<PliShard> const& pliShards,
                                         PredicatePacks const& packs,
                                         util::WorkerThreadPool* thread_pool);
ClueSetT<uint32_t> BuildClueSetParallel32(std::vector<PliShard> const& pliShards,
                                         PredicatePacks const& packs,
                                         util::WorkerThreadPool* thread_pool);
ClueSetT<uint64_t> BuildClueSetParallel64(std::vector<PliShard> const& pliShards,
                                         PredicatePacks const& packs,
                                         util::WorkerThreadPool* thread_pool);

}  // namespace algos::fastadc
