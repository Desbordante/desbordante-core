#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"

#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "util/erase_if_replace.h"

namespace algos::hymd::lattice::cardinality {

std::vector<ValidationInfo> MinPickingLevelGetter::GetCurrentMdsInternal(
        std::vector<MdLattice::MdVerificationMessenger>& level_lattice_info) {
    min_picker_.NewBatch(level_lattice_info.size());
    std::unordered_map<MdLhs, boost::dynamic_bitset<>> new_picked;
    for (MdLattice::MdVerificationMessenger& messenger : level_lattice_info) {
        auto node = picked_.extract(messenger.GetLhs());
        boost::dynamic_bitset<> indices(column_matches_number_);
        lattice::Rhs const& rhs = messenger.GetRhs();
        for (model::Index i = 0; i != column_matches_number_; ++i) {
            if (rhs[i] != kLowestCCValueId) {
                indices.set(i);
            }
        }
        if (node.empty()) {
            min_picker_.AddGeneralizations(messenger, indices);
            continue;
        }
        boost::dynamic_bitset<> const& previously_picked_rhs = node.mapped();
        indices -= previously_picked_rhs;
        new_picked.insert(std::move(node));
        if (indices.none()) continue;
        min_picker_.AddGeneralizations(messenger, indices);
    }
    std::vector<ValidationInfo> collected = min_picker_.GetAll();
    if constexpr (MinPickerType::kNeedsEmptyRemoval) {
        if constexpr (kEraseEmptyKeepOrder) {
            std::erase_if(collected, [](ValidationInfo const& validation_info) {
                return validation_info.rhs_indices.none();
            });
        } else {
            util::EraseIfReplace(collected, [](ValidationInfo const& validation_info) {
                return validation_info.rhs_indices.none();
            });
        }
    }
    for (ValidationInfo const& validation_info : collected) {
        boost::dynamic_bitset<> const& new_rhs_indices = validation_info.rhs_indices;
        auto [it, new_validation] = new_picked.try_emplace(validation_info.messenger->GetLhs(),
                                                           new_rhs_indices);
        if (new_validation) continue;
        boost::dynamic_bitset<>& previously_picked_rhs = it->second;
        assert((previously_picked_rhs & new_rhs_indices).none());
        previously_picked_rhs |= new_rhs_indices;
    }
    if (collected.empty()) {
        picked_.clear();
        ++cur_level_;
    }
    picked_ = std::move(new_picked);
    return collected;
}

}  // namespace algos::hymd::lattice::cardinality
