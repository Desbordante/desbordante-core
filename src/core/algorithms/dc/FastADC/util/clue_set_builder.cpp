#include "core/algorithms/dc/FastADC/util/clue_set_builder.h"

#include <cassert>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/algorithms/dc/FastADC/util/cross_clue_set_builder.h"
#include "core/algorithms/dc/FastADC/util/single_clue_set_builder.h"

namespace algos::fastadc {

namespace {

template <typename ClueT>
ClueSetT<ClueT> BuildClueSetScalar(std::vector<PliShard> const& pli_shards,
                                   PredicatePacks const& packs) {
    if (pli_shards.empty()) {
        return {};
    }

    ClueSetT<ClueT> clue_set;
    ClueSetT<ClueT> partial_clue_set;

    // Range of all pli_shards is equal, so it's safe to pass a pre-allocated vector of
    // pli_shards[0]'s range
    size_t range = pli_shards[0].Range();
    size_t evidence_count = range * range;

    clue_set.reserve(range * 2);
    partial_clue_set.reserve(range * 2);

    std::vector<ClueT> forward_clues(evidence_count, ClueT{});
    std::vector<ClueT> reverse_clues(evidence_count, ClueT{});

    for (size_t i = 0; i < pli_shards.size(); i++) {
        for (size_t j = i; j < pli_shards.size(); j++) {
            if (i == j) {
                SingleClueSetBuilderT<ClueT>{pli_shards[i]}.BuildClueSet(packs, forward_clues,
                                                                        partial_clue_set);
            } else {
                CrossClueSetBuilderT<ClueT>{pli_shards[i], pli_shards[j]}.BuildClueSet(
                        packs, forward_clues, reverse_clues, partial_clue_set);
            }

            for (auto const& [clue, count] : partial_clue_set) {
                auto [it, inserted] = clue_set.emplace(clue, count);
                if (!inserted) {
                    it->second += count;
                }
            }
        }
    }

    return clue_set;
}

std::pair<size_t, size_t> TaskIdToIndices(size_t task_id, size_t n_shards) {
    auto prefix = [n_shards](size_t i) { return (i * (2 * n_shards - i + 1)) / 2; };

    size_t lo = 0;
    size_t hi = n_shards;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (prefix(mid + 1) <= task_id) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    size_t i = lo;
    size_t base = prefix(i);
    size_t offset = task_id - base;
    size_t j = i + offset;

    assert(i < n_shards && j < n_shards && j >= i);

    return {i, j};
}

template <typename ClueT>
struct ThreadLocalBuffersT {
    std::vector<ClueT> forward_clues;
    std::vector<ClueT> reverse_clues;
    ClueSetT<ClueT> task_clue_set;
    ClueSetT<ClueT> thread_clue_set;

    ThreadLocalBuffersT(size_t evidence_count, size_t reserve_size)
        : forward_clues(evidence_count, ClueT{}), reverse_clues(evidence_count, ClueT{}) {
        task_clue_set.reserve(reserve_size);
        thread_clue_set.reserve(reserve_size);
    }
};

template <typename ClueT>
ClueSetT<ClueT> BuildClueSetParallelScalar(std::vector<PliShard> const& pli_shards,
                                           PredicatePacks const& packs,
                                           util::WorkerThreadPool* thread_pool) {
    if (!thread_pool) {
        return BuildClueSetScalar<ClueT>(pli_shards, packs);
    }

    size_t const n_shards = pli_shards.size();
    size_t const task_count = (n_shards * (n_shards + 1)) / 2;

    if (task_count <= 1) {
        return BuildClueSetScalar<ClueT>(pli_shards, packs);
    }

    size_t const range = pli_shards[0].Range();
    size_t const evidence_count = range * range;

    ClueSetT<ClueT> global_clue_set;
    global_clue_set.reserve(range * 2 * n_shards);
    std::mutex global_mutex;

    thread_pool->ExecIndexWithResource(
            [&pli_shards, &packs, n_shards](size_t task_id, ThreadLocalBuffersT<ClueT>& buffers) {
                auto const [i, j] = TaskIdToIndices(task_id, n_shards);
                if (i >= n_shards || j >= n_shards) {
                    throw std::runtime_error("TaskIdToIndices produced out-of-bounds shard index");
                }

                buffers.task_clue_set.clear();

                if (i == j) {
                    SingleClueSetBuilderT<ClueT>{pli_shards[i]}.BuildClueSet(
                            packs, buffers.forward_clues, buffers.task_clue_set);
                } else {
                    CrossClueSetBuilderT<ClueT>{pli_shards[i], pli_shards[j]}.BuildClueSet(
                            packs, buffers.forward_clues, buffers.reverse_clues,
                            buffers.task_clue_set);
                }

                for (auto const& [clue, count] : buffers.task_clue_set) {
                    auto [it, inserted] = buffers.thread_clue_set.emplace(clue, count);
                    if (!inserted) {
                        it->second += count;
                    }
                }
            },
            [range, evidence_count]() {
                return ThreadLocalBuffersT<ClueT>(evidence_count, range * 2);
            },
            task_count,
            [&global_clue_set, &global_mutex](ThreadLocalBuffersT<ClueT>&& buffers) {
                std::lock_guard<std::mutex> lock(global_mutex);
                for (auto const& [clue, count] : buffers.thread_clue_set) {
                    auto [it, inserted] = global_clue_set.emplace(clue, count);
                    if (!inserted) {
                        it->second += count;
                    }
                }
            });

    return global_clue_set;
}

}  // namespace

ClueSet BuildClueSet(std::vector<PliShard> const& pli_shards, PredicatePacks const& packs) {
    return BuildClueSetScalar<Clue>(pli_shards, packs);
}

ClueSet BuildClueSetParallel(std::vector<PliShard> const& pli_shards, PredicatePacks const& packs,
                             util::WorkerThreadPool* thread_pool) {
    return BuildClueSetParallelScalar<Clue>(pli_shards, packs, thread_pool);
}

template <std::size_t Bits>
ClueSetT<model::Bitset<Bits>> BuildClueSetSized(std::vector<PliShard> const& pli_shards,
                                                PredicatePacks const& packs) {
    return BuildClueSetScalar<model::Bitset<Bits>>(pli_shards, packs);
}

template <std::size_t Bits>
ClueSetT<model::Bitset<Bits>> BuildClueSetParallelSized(std::vector<PliShard> const& pli_shards,
                                                       PredicatePacks const& packs,
                                                       util::WorkerThreadPool* thread_pool) {
    return BuildClueSetParallelScalar<model::Bitset<Bits>>(pli_shards, packs, thread_pool);
}

template ClueSetT<model::Bitset<8>> BuildClueSetSized<8>(std::vector<PliShard> const&,
                                                         PredicatePacks const&);
template ClueSetT<model::Bitset<16>> BuildClueSetSized<16>(std::vector<PliShard> const&,
                                                           PredicatePacks const&);
template ClueSetT<model::Bitset<32>> BuildClueSetSized<32>(std::vector<PliShard> const&,
                                                           PredicatePacks const&);
template ClueSetT<model::Bitset<64>> BuildClueSetSized<64>(std::vector<PliShard> const&,
                                                           PredicatePacks const&);

template ClueSetT<model::Bitset<8>> BuildClueSetParallelSized<8>(std::vector<PliShard> const&,
                                                                 PredicatePacks const&,
                                                                 util::WorkerThreadPool*);
template ClueSetT<model::Bitset<16>> BuildClueSetParallelSized<16>(std::vector<PliShard> const&,
                                                                   PredicatePacks const&,
                                                                   util::WorkerThreadPool*);
template ClueSetT<model::Bitset<32>> BuildClueSetParallelSized<32>(std::vector<PliShard> const&,
                                                                   PredicatePacks const&,
                                                                   util::WorkerThreadPool*);
template ClueSetT<model::Bitset<64>> BuildClueSetParallelSized<64>(std::vector<PliShard> const&,
                                                                   PredicatePacks const&,
                                                                   util::WorkerThreadPool*);

}  // namespace algos::fastadc
