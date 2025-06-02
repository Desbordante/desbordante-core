#pragma once

#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"
#include "algorithms/md/hymd/lattice/cardinality/one_by_one_min_picker.h"
#include "algorithms/md/hymd/lattice/level_getter.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice::cardinality {

class MinPickingLevelGetter final : public LevelGetter {
private:
    using MinPickerType = OneByOnePicker;
    std::size_t const column_matches_number_;
    MinPickerType min_picker_;
    std::unordered_map<MdLhs, boost::dynamic_bitset<>> picked_;

    std::vector<ValidationInfo> GetPendingGroupedMinimalLhsMds(
            std::vector<MdLattice::MdVerificationMessenger>& level_lattice_info) final;

    // true simulates Metanome, false makes the procedure faster, but the order changes may lead to
    // unpredictable effects on runtime
    static constexpr bool kEraseEmptyKeepOrder = false;

public:
    MinPickingLevelGetter(MdLattice* lattice)
        : LevelGetter(lattice),
          column_matches_number_(lattice->GetColMatchNumber()),
          min_picker_() {}
};

}  // namespace algos::hymd::lattice::cardinality
