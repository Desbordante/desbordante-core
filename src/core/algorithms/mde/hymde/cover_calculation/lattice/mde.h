#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"

namespace algos::hymde::cover_calculation::lattice {
struct Mde {
    using FasterType = void;

    MdeLhs const& lhs;
    MdeElement rhs;

    auto& GetRhs() const noexcept {
        return rhs;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
