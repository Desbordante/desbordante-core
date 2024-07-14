#pragma once

#include "algorithms/md/hymd/lattice/lhs_specialization.h"
#include "algorithms/md/hymd/lattice/md.h"
#include "algorithms/md/hymd/lattice/multi_md.h"
#include "algorithms/md/hymd/md_element.h"
#include "algorithms/md/hymd/utility/exclusion_list.h"

namespace algos::hymd::lattice {
struct MultiMdSpecialization {
    using Unspecialized = MultiMd;

    LhsSpecialization const& lhs_specialization;
    utility::ExclusionList<MdElement>& rhss;

    Unspecialized ToUnspecialized() const noexcept {
        return {lhs_specialization.ToUnspecialized(), rhss};
    }

    LhsSpecialization const& GetLhsSpecialization() const noexcept {
        return lhs_specialization;
    }

    utility::ExclusionList<MdElement> const& GetRhs() const noexcept {
        return rhss;
    }
};
}  // namespace algos::hymd::lattice
