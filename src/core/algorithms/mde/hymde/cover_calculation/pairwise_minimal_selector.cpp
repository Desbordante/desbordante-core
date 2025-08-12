#include "algorithms/mde/hymde/cover_calculation/pairwise_minimal_selector.h"

namespace algos::hymde::cover_calculation {
void PairwiseMinimalSelector::NewBatch(std::size_t elements) {
    currently_picked_.clear();
    currently_picked_.reserve(elements);
}

// Assuming [gen_it, gen_end) does not specialize [spec_it, spec_end), is the former a
// generalization of the latter?
bool PairwiseMinimalSelector::IsGeneralization(MdeLhs::iterator gen_it, MdeLhs::iterator spec_it,
                                               MdeLhs::iterator const gen_end,
                                               MdeLhs::iterator const spec_end) {
    using model::Index;
    auto inc_until_eq = [](MdeLhs::iterator const gen_it, MdeLhs::iterator& spec_it,
                           MdeLhs::iterator const spec_end) {
        auto const [gen_index, gen_rcv_id] = *gen_it;
        Index cur_spec_index = 0;
        do {
            auto const& [spec_child_index, spec_rcv_id] = *spec_it;
            cur_spec_index += spec_child_index;
            // Incomparable, spec has no classifier for the considered gen's record match.
            if (cur_spec_index > gen_index) return true;
            if (cur_spec_index == gen_index) {
                // Incomparable, spec's classifier matches more record pairs.
                if (gen_rcv_id > spec_rcv_id) return true;
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

auto PairwiseMinimalSelector::CompareLhss(MdeLhs const& cur, MdeLhs const& prev)
        -> ComparisonResult {
    MdeLhs::iterator cur_it = cur.begin();
    MdeLhs::iterator const cur_end = cur.end();
    MdeLhs::iterator prev_it = prev.begin();
    MdeLhs::iterator const prev_end = prev.end();
    while (cur_it != cur_end) {     // cur still has LHS elements.
        if (prev_it == prev_end) {  // prev has no more LHS elements, prev generalizes cur.
            return ComparisonResult::kSpecialization;
        }
        auto const& [cur_child_index, cur_rcv_id] = *cur_it;
        auto const& [prev_child_index, prev_rcv_id] = *prev_it;
        if (cur_child_index > prev_child_index) {  // prev cannot be a generalization of cur.
            if (IsGeneralization(cur_it, prev_it, cur_end, prev_end))
                return ComparisonResult::kGeneralization;
            return ComparisonResult::kIncomparable;
        } else if (cur_child_index < prev_child_index) {  // cur cannot be a generalization of prev.
            if (IsGeneralization(prev_it, cur_it, prev_end, cur_end))
                return ComparisonResult::kSpecialization;
            return ComparisonResult::kIncomparable;
        } else if (cur_rcv_id < prev_rcv_id) {  // prev cannot be a generalization of cur.
            if (IsGeneralizationIncr(cur_it, prev_it, cur_end, prev_end))
                return ComparisonResult::kGeneralization;
            return ComparisonResult::kIncomparable;
        } else if (prev_rcv_id < cur_rcv_id) {  // cur cannot be a generalization of prev.
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

void PairwiseMinimalSelector::AddGeneralizations(lattice::MdeLattice::ValidationUpdater& updater,
                                                 boost::dynamic_bitset<>& considered_indices) {
    MdeLhs const& lhs_cur = updater.GetLhs();
    assert(lhs_cur.PathLength() != 0 || currently_picked_.empty());
    for (ValidationSelection& prev_selection : currently_picked_) {
        MdeLhs const& lhs_prev = prev_selection.updater->GetLhs();
        boost::dynamic_bitset<>& indices_prev = prev_selection.rhs_indices_to_validate;
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
    currently_picked_.push_back({&updater, std::move(considered_indices)});
}

std::vector<ValidationSelection> PairwiseMinimalSelector::GetAll() noexcept {
    static_assert(kNeedsEmptyRemoval,
                  "This picker needs post-processing to remove candidates with empty RHS indices");
    return std::move(currently_picked_);
}
}  // namespace algos::hymde::cover_calculation
