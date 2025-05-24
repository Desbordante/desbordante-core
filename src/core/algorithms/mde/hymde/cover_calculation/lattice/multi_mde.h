#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/mde.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"
#include "algorithms/mde/hymde/utility/exclusion_list.h"

namespace algos::hymde::cover_calculation::lattice {
struct MultiMde {
    using FasterType = Mde;

    MdeLhs const& lhs;
    utility::ExclusionList<MdeElement>& rhss;

    bool ShouldConvert() const noexcept {
        return rhss.GetEnabled().count() == 1;
    }

    FasterType ToFasterType() const noexcept {
        return {lhs, rhss.GetElements()[rhss.GetEnabled().find_first()]};
    }

    utility::ExclusionList<MdeElement>& GetRhs() noexcept {
        return rhss;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
