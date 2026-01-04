#pragma once

#include "core/algorithms/md/hymd/lattice/md.h"
#include "core/algorithms/md/hymd/md_element.h"
#include "core/algorithms/md/hymd/md_lhs.h"
#include "core/algorithms/md/hymd/utility/exclusion_list.h"

namespace algos::hymd::lattice {
struct MultiMd {
    using FasterType = Md;

    MdLhs const& lhs;
    utility::ExclusionList<MdElement>& rhss;

    bool ShouldConvert() const noexcept {
        return rhss.GetEnabled().count() == 1;
    }

    FasterType ToFasterType() const noexcept {
        return {lhs, rhss.GetElements()[rhss.GetEnabled().find_first()]};
    }

    utility::ExclusionList<MdElement>& GetRhs() noexcept {
        return rhss;
    }
};
}  // namespace algos::hymd::lattice
