#pragma once

#include "algorithms/md/hymd/indexes/dictionary_compressor.h"
#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"
#include "algorithms/md/hymd/lattice/level_getter.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/validator.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class LatticeTraverser {
private:
    struct LatticeStatistics {
        std::size_t all_mds_num = 0;
        std::size_t invalidated_mds_num = 0;
        static constexpr double kRatioBound = 0.01;
    };

    class ClearingRecRef {
        Recommendations& recommendations_;

    public:
        ClearingRecRef(Recommendations& recommendations) : recommendations_(recommendations) {}

        operator Recommendations const&() const {
            return recommendations_;
        }

        ~ClearingRecRef() {
            recommendations_.clear();
        }
    };

    Recommendations recommendations_;

    lattice::LevelGetter& level_getter_;
    Validator validator_;

    util::WorkerThreadPool* pool_;

public:
    LatticeTraverser(lattice::LevelGetter& level_getter, Validator validator,
                     util::WorkerThreadPool* pool) noexcept
        : level_getter_(level_getter), validator_(std::move(validator)), pool_(pool) {}

    bool TraverseLattice(bool traverse_all);

    ClearingRecRef TakeRecommendations() noexcept {
        return recommendations_;
    }
};

}  // namespace algos::hymd
