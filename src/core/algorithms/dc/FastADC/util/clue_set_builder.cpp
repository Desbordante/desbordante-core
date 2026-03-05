#include "core/algorithms/dc/FastADC/util/clue_set_builder.h"

#include <cassert>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "core/algorithms/dc/FastADC/util/cross_clue_set_builder.h"
#include "core/algorithms/dc/FastADC/util/single_clue_set_builder.h"

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs) {
    if (pliShards.empty()) {
        return {};
    }

    ClueSet clue_set;
    ClueSet partial_clue_set;

    size_t task_count = (pliShards.size() * (pliShards.size() + 1)) / 2;
    LOG_DEBUG("  [CLUE] task count: {}", task_count);

    // Range of all pliShards is equal, so it's safe to pass a pre-allocated vector of
    // pliShards[0]'s range
    size_t range = pliShards[0].Range();
    size_t evidence_count = range * range;

    clue_set.reserve(range * 2);
    partial_clue_set.reserve(range * 2);

    std::vector<Clue> forward_clues(evidence_count, 0);
    std::vector<Clue> reverse_clues(evidence_count, 0);

    for (size_t i = 0; i < pliShards.size(); i++) {
        for (size_t j = i; j < pliShards.size(); j++) {
            if (i == j) {
                SingleClueSetBuilder{pliShards[i]}.BuildClueSet(packs, forward_clues,
                                                                partial_clue_set);
            } else {
                CrossClueSetBuilder{pliShards[i], pliShards[j]}.BuildClueSet(
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

namespace {

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

struct ThreadLocalBuffers {
    std::vector<Clue> forward_clues;
    std::vector<Clue> reverse_clues;
    ClueSet task_clue_set;
    ClueSet thread_clue_set;

    ThreadLocalBuffers(size_t evidence_count, size_t reserve_size)
        : forward_clues(evidence_count, 0), reverse_clues(evidence_count, 0) {
        task_clue_set.reserve(reserve_size);
        thread_clue_set.reserve(reserve_size);
    }
};

}  // namespace

ClueSet BuildClueSetParallel(std::vector<PliShard> const& pliShards, PredicatePacks const& packs,
                             util::WorkerThreadPool* thread_pool) {
    size_t const n_shards = pliShards.size();
    size_t const task_count = (n_shards * (n_shards + 1)) / 2;

    if (!thread_pool || task_count <= 1) {
        return BuildClueSet(pliShards, packs);
    }

    size_t const range = pliShards[0].Range();
    size_t const evidence_count = range * range;

    ClueSet global_clue_set;
    global_clue_set.reserve(range * 2 * n_shards);
    std::mutex global_mutex;

    thread_pool->ExecIndexWithResource(
            [&pliShards, &packs, n_shards](size_t task_id, ThreadLocalBuffers& buffers) {
                auto const [i, j] = TaskIdToIndices(task_id, n_shards);
                if (i >= n_shards || j >= n_shards) {
                    throw std::runtime_error("TaskIdToIndices produced out-of-bounds shard index");
                }

                buffers.task_clue_set.clear();

                if (i == j) {
                    SingleClueSetBuilder{pliShards[i]}.BuildClueSet(packs, buffers.forward_clues,
                                                                    buffers.task_clue_set);
                } else {
                    CrossClueSetBuilder{pliShards[i], pliShards[j]}.BuildClueSet(
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
            [range, evidence_count]() { return ThreadLocalBuffers(evidence_count, range * 2); },
            task_count,
            [&global_clue_set, &global_mutex](ThreadLocalBuffers&& buffers) {
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

}  // namespace algos::fastadc
