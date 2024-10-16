#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"

namespace algos::hymd::lattice {

class LevelGetter {
    // Store here, use pointers elsewhere.
    std::vector<MdLattice::MdVerificationMessenger> messengers_;
    MdLattice* const lattice_;

    virtual std::vector<ValidationInfo> GetPendingGroupedMinimalLhsMds(
            std::vector<MdLattice::MdVerificationMessenger>& level_mds) = 0;

    std::size_t cur_level_ = 0;

protected:
    void NextLevel() noexcept {
        ++cur_level_;
    }

public:
    LevelGetter(MdLattice* lattice) : lattice_(lattice) {}

    std::vector<ValidationInfo> GetPendingGroupedMinimalLhsMds() {
        while (cur_level_ <= lattice_->GetMaxLevel()) {
            messengers_ = lattice_->GetLevel(cur_level_);
            std::vector<ValidationInfo> validations = GetPendingGroupedMinimalLhsMds(messengers_);
            if (!validations.empty()) return validations;
        }
        return {};
    }

    virtual ~LevelGetter() = default;
};

}  // namespace algos::hymd::lattice
