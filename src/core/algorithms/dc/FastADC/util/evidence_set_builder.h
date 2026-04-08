#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "core/algorithms/dc/FastADC/model/evidence_set.h"
#include "core/algorithms/dc/FastADC/util/clue_set_builder.h"
#include "core/util/logger.h"
#include "core/util/worker_thread_pool.h"

namespace algos::fastadc {

class EvidenceSetBuilder {
public:
    EvidenceSet evidence_set;

    EvidenceSetBuilder(std::vector<PliShard> const& pli_shards, PredicatePacks const& packs,
                       size_t clue_bit_count, util::WorkerThreadPool* thread_pool = nullptr) {
        if (clue_bit_count > kMaxPredicateBits) {
            throw std::invalid_argument(
                    "FastADC: predicate space is too large (clue bits exceed maximum supported).");
        }

        auto consume_scalar = [this](auto const& clue_set) {
            clues_.reserve(clue_set.size());
            for (auto const& [clue, count] : clue_set) {
                clues_.emplace_back(PredicateBitset(static_cast<uint64_t>(clue)), count);
            }
        };

        if (clue_bit_count <= 8) {
            auto clue_set = thread_pool ? BuildClueSetParallel8(pli_shards, packs, thread_pool)
                                        : BuildClueSet8(pli_shards, packs);
            consume_scalar(clue_set);
        } else if (clue_bit_count <= 16) {
            auto clue_set = thread_pool ? BuildClueSetParallel16(pli_shards, packs, thread_pool)
                                        : BuildClueSet16(pli_shards, packs);
            consume_scalar(clue_set);
        } else if (clue_bit_count <= 32) {
            auto clue_set = thread_pool ? BuildClueSetParallel32(pli_shards, packs, thread_pool)
                                        : BuildClueSet32(pli_shards, packs);
            consume_scalar(clue_set);
        } else if (clue_bit_count <= 64) {
            auto clue_set = thread_pool ? BuildClueSetParallel64(pli_shards, packs, thread_pool)
                                        : BuildClueSet64(pli_shards, packs);
            consume_scalar(clue_set);
        } else {
            LOG_WARN(
                    "Using 128-bit representation for clues ({} bits required). Performance may be "
                    "degraded.",
                    clue_bit_count);
            auto clue_set = thread_pool ? BuildClueSetParallel(pli_shards, packs, thread_pool)
                                        : BuildClueSet(pli_shards, packs);
            clues_.reserve(clue_set.size());
            for (auto const& [clue, count] : clue_set) {
                clues_.emplace_back(clue, count);
            }
        }
    }

    EvidenceSetBuilder(EvidenceSetBuilder const& other) = delete;
    EvidenceSetBuilder& operator=(EvidenceSetBuilder const& other) = delete;
    EvidenceSetBuilder(EvidenceSetBuilder&& other) noexcept = default;
    EvidenceSetBuilder& operator=(EvidenceSetBuilder&& other) noexcept = delete;

    void BuildEvidenceSet(std::vector<PredicateBitset> const& correction_map,
                          PredicateBitset const& cardinality_mask) {
        evidence_set.Reserve(clues_.size());

        for (auto const& [clue, count] : clues_) {
            evidence_set.EmplaceBack(clue, count, cardinality_mask, correction_map);
        }

        LOG_DEBUG(" [Evidence] # of evidences: {}", evidence_set.Size());
        LOG_DEBUG(" [Evidence] Accumulated evidence count: {}", evidence_set.GetTotalCount());
    }

private:
    std::vector<std::pair<PredicateBitset, int64_t>> clues_;
};

}  // namespace algos::fastadc
