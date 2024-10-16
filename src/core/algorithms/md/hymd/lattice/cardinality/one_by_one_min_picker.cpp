#include "algorithms/md/hymd/lattice/cardinality/one_by_one_min_picker.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/md_lhs.h"
#include "util/desbordante_assume.h"

namespace algos::hymd::lattice::cardinality {

void OneByOnePicker::NewBatch(std::size_t elements) {
    currently_picked_.clear();
    currently_picked_.reserve(elements);
}

// Assuming [gen_it, gen_end) does not specialize [spec_it, spec_end), is the former a
// generalization of the latter?
bool OneByOnePicker::IsGeneralization(MdLhs::iterator gen_it, MdLhs::iterator spec_it,
                                      MdLhs::iterator const gen_end,
                                      MdLhs::iterator const spec_end) {
    using model::Index;
    auto inc_until_eq = [](MdLhs::iterator const gen_it, MdLhs::iterator& spec_it,
                           MdLhs::iterator const spec_end) {
        auto const [gen_index, gen_ccv_id] = *gen_it;
        Index cur_spec_index = 0;
        do {
            auto const& [spec_child_index, spec_ccv_id] = *spec_it;
            cur_spec_index += spec_child_index;
            // Incomparable, spec has no classifier for the considered gen's column match.
            if (cur_spec_index > gen_index) return true;
            if (cur_spec_index == gen_index) {
                // Incomparable, spec's classifier matches more record pairs.
                if (gen_ccv_id > spec_ccv_id) return true;
                return false;
            }
            ++cur_spec_index;
        } while (++spec_it != spec_end);
        // Incomparable, generalization's node is after the last specialization's node.
        return true;
    };
    while (true) {
        bool is_incomparable = inc_until_eq(gen_it, spec_it, spec_end);
        if (is_incomparable) return false;
        // Fewer classifiers in gen, gen generalizes spec.
        if (++gen_it == gen_end) return true;
        // More classifiers in gen, gen is incomparable with spec.
        if (++spec_it == spec_end) return false;
    }
}

auto OneByOnePicker::CompareLhss(MdLhs const& cur, MdLhs const& prev) -> ComparisonResult {
    MdLhs::iterator cur_it = cur.begin();
    MdLhs::iterator const cur_end = cur.end();
    MdLhs::iterator prev_it = prev.begin();
    MdLhs::iterator const prev_end = prev.end();
    while (cur_it != cur_end) {     // cur still has LHS elements.
        if (prev_it == prev_end) {  // prev has no more LHS elements, prev generalizes cur.
            return ComparisonResult::kSpecialization;
        }
        auto const& [cur_child_index, cur_ccv_id] = *cur_it;
        auto const& [prev_child_index, prev_ccv_id] = *prev_it;
        if (cur_child_index > prev_child_index) {  // prev cannot be a generalization of cur.
            if (IsGeneralization(cur_it, prev_it, cur_end, prev_end))
                return ComparisonResult::kGeneralization;
            return ComparisonResult::kIncomparable;
        } else if (cur_child_index < prev_child_index) {  // cur cannot be a generalization of prev.
            if (IsGeneralization(prev_it, cur_it, prev_end, cur_end))
                return ComparisonResult::kSpecialization;
            return ComparisonResult::kIncomparable;
        } else if (cur_ccv_id < prev_ccv_id) {  // prev cannot be a generalization of cur.
            if (IsGeneralizationIncr(cur_it, prev_it, cur_end, prev_end))
                return ComparisonResult::kGeneralization;
            return ComparisonResult::kIncomparable;
        } else if (prev_ccv_id < cur_ccv_id) {  // cur cannot be a generalization of prev.
            if (IsGeneralizationIncr(prev_it, cur_it, prev_end, cur_end))
                return ComparisonResult::kSpecialization;
            return ComparisonResult::kIncomparable;
        }
        ++cur_it;
        ++prev_it;
    }
    // Assuming all LHSs are distinct.
    assert(prev_it != prev_end);
    // cur has no more LHS elements, cur generalizes prev.
    return ComparisonResult::kGeneralization;
}

void OneByOnePicker::AddGeneralizations(MdLattice::MdVerificationMessenger& messenger,
                                        boost::dynamic_bitset<>& considered_indices) {
    MdLhs const& lhs_cur = messenger.GetLhs();
    assert(!lhs_cur.IsEmpty() || currently_picked_.empty());
    for (ValidationInfo& prev_info : currently_picked_) {
        MdLhs const& lhs_prev = prev_info.messenger->GetLhs();
        boost::dynamic_bitset<>& indices_prev = prev_info.rhs_indices_to_validate;
        switch (CompareLhss(lhs_cur, lhs_prev)) {
            case ComparisonResult::kSpecialization:
                considered_indices -= indices_prev;
                if (considered_indices.none()) return;
                break;
            case ComparisonResult::kGeneralization:
                indices_prev -= considered_indices;
                break;
            case ComparisonResult::kIncomparable:
                break;
            default:
                DESBORDANTE_ASSUME(false);
        }
    }
    assert(!considered_indices.none());
    currently_picked_.emplace_back(&messenger, std::move(considered_indices));
}

std::vector<ValidationInfo> OneByOnePicker::GetAll() noexcept {
    static_assert(kNeedsEmptyRemoval,
                  "This picker needs post-processing to remove candidates with empty RHS indices");
    return std::move(currently_picked_);
}

}  // namespace algos::hymd::lattice::cardinality
