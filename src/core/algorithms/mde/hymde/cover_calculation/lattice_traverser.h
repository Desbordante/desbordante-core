#pragma once

#include <cstddef>

#include "algorithms/mde/hymde/cover_calculation/batch_validator.h"
#include "algorithms/mde/hymde/cover_calculation/level_getter.h"
#include "algorithms/mde/hymde/cover_calculation/recommendation.h"
#include "algorithms/mde/hymde/cover_calculation/validation_selection.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::cover_calculation {
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

        void CountOne(ValidationSelection& validation, BatchValidator::Result const& result) {
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

    LevelGetter& level_getter_;
    BatchValidator validator_;

    util::WorkerThreadPool* pool_;

    void AddRecommendations(std::vector<BatchValidator::Result> const& results);
    LatticeStatistics AdjustLattice(std::vector<ValidationSelection>& validations,
                                    std::vector<BatchValidator::Result> const& results);
    LatticeStatistics ProcessResults(std::vector<ValidationSelection>& validations,
                                     std::vector<BatchValidator::Result> const& results);

public:
    LatticeTraverser(LevelGetter& level_getter, BatchValidator validator,
                     util::WorkerThreadPool* pool) noexcept
        : level_getter_(level_getter), validator_(std::move(validator)), pool_(pool) {}

    bool TraverseLattice(bool traverse_all);

    ClearingRecRef TakeRecommendations() noexcept {
        return recommendations_;
    }
};
}  // namespace algos::hymde::cover_calculation
