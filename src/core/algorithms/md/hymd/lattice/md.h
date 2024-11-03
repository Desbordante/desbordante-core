#pragma once

#include "algorithms/md/hymd/md_element.h"
#include "algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice {
struct Md {
    using FasterType = void;

    MdLhs const& lhs;
    MdElement rhs;

    auto& GetRhs() const noexcept {
        return rhs;
    }
};
}  // namespace algos::hymd::lattice
