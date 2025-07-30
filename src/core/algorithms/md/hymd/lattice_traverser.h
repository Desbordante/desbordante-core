#pragma once

#include <cstddef>  // for size_t
#include <utility>  // for move
#include <vector>   // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset

#include "algorithms/md/hymd/recommendation.h"  // for Recommendations
#include "algorithms/md/hymd/validator.h"       // for BatchValidator
#include "md/hymd/lattice/validation_info.h"    // for ValidationInfo
#include "md/hymd/utility/invalidated_rhss.h"   // for InvalidatedRhss

namespace algos {
namespace hymd {
namespace lattice {
class LevelGetter;
}
}  // namespace hymd
}  // namespace algos

namespace util {
class WorkerThreadPool;
}

namespace algos::hymd {

class LatticeTraverser {
private:
    class LatticeStatistics {
        std::size_t all_mds_num_ = 0;
        std::size_t invalidated_mds_num_ = 0;
        static constexpr double kRatioBound = 0.01;

    public:
        bool TraversalInefficient() const noexcept {
            return invalidated_mds_num_ > kRatioBound * (all_mds_num_ - invalidated_mds_num_);
        }

        void CountOne(lattice::ValidationInfo& validation, BatchValidator::Result const& result) {
            invalidated_mds_num_ += result.invalidated_rhss.Size();
            all_mds_num_ += validation.rhs_indices_to_validate.count();
        }
    };

    class ClearingRecRef {
        Recommendations& recommendations_;

    public:
        ClearingRecRef(Recommendations& recommendations) noexcept
            : recommendations_(recommendations) {}

        operator Recommendations const&() const noexcept {
            return recommendations_;
        }

        ~ClearingRecRef() noexcept {
            recommendations_.clear();
        }
    };

    Recommendations recommendations_;

    lattice::LevelGetter& level_getter_;
    BatchValidator validator_;

    util::WorkerThreadPool* pool_;

    void AddRecommendations(std::vector<BatchValidator::Result> const& results);
    static LatticeStatistics AdjustLattice(std::vector<lattice::ValidationInfo>& validations,
                                           std::vector<BatchValidator::Result> const& results);
    LatticeStatistics ProcessResults(std::vector<lattice::ValidationInfo>& validations,
                                     std::vector<BatchValidator::Result> const& results);

public:
    LatticeTraverser(lattice::LevelGetter& level_getter, BatchValidator validator,
                     util::WorkerThreadPool* pool) noexcept
        : level_getter_(level_getter), validator_(std::move(validator)), pool_(pool) {}

    bool TraverseLattice(bool traverse_all);

    ClearingRecRef TakeRecommendations() noexcept {
        return recommendations_;
    }
};

}  // namespace algos::hymd
