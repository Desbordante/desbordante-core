#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/lhs_specialization.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"

namespace algos::hymde::cover_calculation::lattice {
struct MdeSpecialization {
    using Unspecialized = Mde;

    LhsSpecialization lhs_specialization;
    MdeElement rhs;

    Unspecialized ToUnspecialized() const noexcept {
        return {lhs_specialization.ToUnspecialized(), rhs};
    }

    LhsSpecialization& GetLhsSpecialization() noexcept {
        return lhs_specialization;
    }

    MdeElement const& GetRhs() const noexcept {
        return rhs;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
