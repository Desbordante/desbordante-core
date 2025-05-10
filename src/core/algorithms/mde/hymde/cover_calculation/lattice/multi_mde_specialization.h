#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/lhs_specialization.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/multi_mde.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"
#include "algorithms/mde/hymde/utility/exclusion_list.h"

namespace algos::hymde::cover_calculation::lattice {
struct MultiMdeSpecialization {
    using Unspecialized = MultiMde;

    LhsSpecialization lhs_specialization;
    utility::ExclusionList<MdeElement>& rhss;

    Unspecialized ToUnspecialized() const noexcept {
        return {lhs_specialization.ToUnspecialized(), rhss};
    }

    LhsSpecialization& GetLhsSpecialization() noexcept {
        return lhs_specialization;
    }

    utility::ExclusionList<MdeElement>& GetRhs() noexcept {
        return rhss;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
