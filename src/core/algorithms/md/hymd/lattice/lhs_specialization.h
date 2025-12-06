#pragma once

#include "core/algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice {
struct SpecializationData {
    MdLhs::iterator spec_before;
    LhsNode new_child;
};

struct LhsSpecialization {
    using Unspecialized = MdLhs const&;

    Unspecialized old_lhs;
    SpecializationData specialization_data;

    Unspecialized ToUnspecialized() const noexcept {
        return old_lhs;
    }

    LhsSpecialization& GetLhsSpecialization() noexcept {
        return *this;
    }
};
}  // namespace algos::hymd::lattice
