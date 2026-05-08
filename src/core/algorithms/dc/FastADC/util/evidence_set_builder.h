#pragma once

#include <cstddef>
#include <stdexcept>
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

        auto consume = [this](auto const& clue_set) {
            clues_.reserve(clue_set.size());
            for (auto const& [clue, count] : clue_set) {
                PredicateBitset bitset;
                for (size_t pos = clue._Find_first(); pos < clue.size();
                     pos = clue._Find_next(pos)) {
                    bitset.set(pos);
                }
                clues_.emplace_back(bitset, count);
            }
        };

        auto run_sized = [&]<std::size_t Bits>() {
            auto clue_set = thread_pool
                                    ? BuildClueSetParallelSized<Bits>(pli_shards, packs, *thread_pool)
                                    : BuildClueSetSized<Bits>(pli_shards, packs);
            consume(clue_set);
        };

        if (clue_bit_count <= 8) {
            run_sized.template operator()<8>();
        } else if (clue_bit_count <= 16) {
            run_sized.template operator()<16>();
        } else if (clue_bit_count <= 32) {
            run_sized.template operator()<32>();
        } else if (clue_bit_count <= 64) {
            run_sized.template operator()<64>();
        } else {
            LOG_WARN(
                    "Using {}-bit representation for clues ({} bits required). Performance may be "
                    "degraded.",
                    static_cast<size_t>(kMaxPredicateBits), clue_bit_count);
            auto clue_set = thread_pool ? BuildClueSetParallel(pli_shards, packs, *thread_pool)
                                        : BuildClueSet(pli_shards, packs);
            consume(clue_set);
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
