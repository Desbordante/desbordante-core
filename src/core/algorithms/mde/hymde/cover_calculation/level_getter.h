#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"
#include "algorithms/mde/hymde/cover_calculation/validation_selection.h"

namespace algos::hymde::cover_calculation {
class LevelGetter {
    // Store here, use pointers elsewhere.
    std::vector<lattice::MdeLattice::ValidationUpdater> updaters_;
    lattice::MdeLattice* const lattice_;

    virtual std::vector<ValidationSelection> GetPendingGroupedMinimalLhsMds(
            std::vector<lattice::MdeLattice::ValidationUpdater>& level_mds) = 0;

    std::size_t cur_level_ = 0;

protected:
    void NextLevel() noexcept {
        ++cur_level_;
    }

public:
    LevelGetter(lattice::MdeLattice* lattice) : lattice_(lattice) {}

    std::vector<ValidationSelection> GetPendingGroupedMinimalLhsMds() {
        while (cur_level_ <= lattice_->GetMaxLevel()) {
            updaters_ = lattice_->GetLevel(cur_level_);
            std::vector<ValidationSelection> validations =
                    GetPendingGroupedMinimalLhsMds(updaters_);
            if (!validations.empty()) return validations;
        }
        return {};
    }

    virtual ~LevelGetter() = default;
};
}  // namespace algos::hymde::cover_calculation
