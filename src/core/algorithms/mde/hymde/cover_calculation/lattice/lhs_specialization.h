#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"

namespace algos::hymde::cover_calculation::lattice {
struct SpecializationData {
    MdeLhs::iterator spec_before;
    LhsNode new_child;
};

struct LhsSpecialization {
    using Unspecialized = MdeLhs const&;

    Unspecialized old_lhs;
    SpecializationData specialization_data;

    Unspecialized ToUnspecialized() const noexcept {
        return old_lhs;
    }

    LhsSpecialization& GetLhsSpecialization() noexcept {
        return *this;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
