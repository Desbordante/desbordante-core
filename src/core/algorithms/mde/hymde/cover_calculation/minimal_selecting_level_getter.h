#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/level_getter.h"
#include "algorithms/mde/hymde/cover_calculation/pairwise_minimal_selector.h"

namespace algos::hymde::cover_calculation {
class MinimalSelectingLevelGetter final : public LevelGetter {
private:
    using MinPickerType = PairwiseMinimalSelector;
    std::size_t const column_matches_number_;
    MinPickerType min_selector_;
    std::unordered_map<lattice::MdeLhs, boost::dynamic_bitset<>> next_selection_exclusion_;

    std::vector<ValidationSelection> GetPendingGroupedMinimalLhsMds(
            std::vector<lattice::MdeLattice::ValidationUpdater>& validation_updaters) final;

    // true simulates Metanome, false makes the procedure faster, but the order changes may lead to
    // unpredictable effects on runtime
    static constexpr bool kEraseEmptyKeepOrder = false;

public:
    MinimalSelectingLevelGetter(lattice::MdeLattice* lattice)
        : LevelGetter(lattice),
          column_matches_number_(lattice->GetColMatchNumber()),
          min_selector_() {}
};
}  // namespace algos::hymde::cover_calculation
