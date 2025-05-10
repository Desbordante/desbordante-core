#include "algorithms/mde/hymde/cover_calculation/minimal_selecting_level_getter.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "model/index.h"
#include "util/erase_if_replace.h"

namespace algos::hymde::cover_calculation {
std::vector<ValidationSelection> MinimalSelectingLevelGetter::GetPendingGroupedMinimalLhsMds(
        std::vector<lattice::MdeLattice::ValidationUpdater>& validation_updaters) {
    min_selector_.NewBatch(validation_updaters.size());
    std::unordered_map<lattice::MdeLhs, boost::dynamic_bitset<>> new_selection_exclusion;
    for (lattice::MdeLattice::ValidationUpdater& messenger : validation_updaters) {
        auto node = next_selection_exclusion_.extract(messenger.GetLhs());
        boost::dynamic_bitset<> indices(column_matches_number_);
        lattice::Rhs const& rhs = messenger.GetRhs();
        for (model::Index i = 0; i != column_matches_number_; ++i) {
            if (rhs[i] != kLowestRCValueId) {
                indices.set(i);
            }
        }
        if (node.empty()) {
            min_selector_.AddGeneralizations(messenger, indices);
            continue;
        }
        boost::dynamic_bitset<> const& previously_picked_rhs = node.mapped();
        indices -= previously_picked_rhs;
        new_selection_exclusion.insert(std::move(node));
        if (indices.none()) continue;
        min_selector_.AddGeneralizations(messenger, indices);
    }
    std::vector<ValidationSelection> collected = min_selector_.GetAll();
    if constexpr (MinPickerType::kNeedsEmptyRemoval) {
        if constexpr (kEraseEmptyKeepOrder) {
            std::erase_if(collected, [](ValidationSelection const& selection) {
                return selection.rhs_indices_to_validate.none();
            });
        } else {
            util::EraseIfReplace(collected, [](ValidationSelection const& selection) {
                return selection.rhs_indices_to_validate.none();
            });
        }
    }
    for (ValidationSelection const& selection : collected) {
        boost::dynamic_bitset<> const& new_rhs_selection = selection.rhs_indices_to_validate;
        auto [it, new_lhs] =
                new_selection_exclusion.try_emplace(selection.updater->GetLhs(), new_rhs_selection);
        if (new_lhs) continue;
        boost::dynamic_bitset<>& previously_selected_rhs = it->second;
        assert((previously_selected_rhs & new_rhs_selection).none());
        previously_selected_rhs |= new_rhs_selection;
    }
    if (collected.empty()) {
        next_selection_exclusion_.clear();
        NextLevel();
    }
    // This is done to save memory. The idea behind this is that there are only two possible
    // situations where we get an MDE from the lattice: either it has yet to be validated or it
    // already was validated. If it is the latter, we want to filter it, this is the only purpose
    // behind this class. In the latter situation we already have it in next_selection_exclusion_.
    // But another reason we could have an MDE in next_selection_exclusion_ is that it was added in
    // the former situation beforehand. If an LHS does not appear again, then, due to lattice
    // invariants, it can only mean that all the previously collected MDEs with said LHS were
    // invalidated and so will never be collected again. Thus, we do not have to remember that MDEs
    // with that LHS were ever collected. Note, it is possible for an MDE with the same LHS as one
    // of the "forgotten" ones to appear, but it will have a different RHS, so that doesn't matter.
    next_selection_exclusion_ = std::move(new_selection_exclusion);
    return collected;
}
}  // namespace algos::hymde::cover_calculation
