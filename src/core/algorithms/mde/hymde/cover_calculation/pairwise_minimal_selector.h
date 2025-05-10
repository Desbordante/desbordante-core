#pragma once

#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/validation_selection.h"

namespace algos::hymde::cover_calculation {
class PairwiseMinimalSelector {
private:
    using MdeLhs = lattice::MdeLhs;

    enum class ComparisonResult {
        kSpecialization,
        kGeneralization,
        kIncomparable,
    };

    std::vector<ValidationSelection> currently_picked_;

    static bool IsGeneralization(MdeLhs::iterator gen_it, MdeLhs::iterator spec_it,
                                 MdeLhs::iterator gen_end, MdeLhs::iterator spec_end);

    static bool IsGeneralizationIncr(MdeLhs::iterator gen_it, MdeLhs::iterator spec_it,
                                     MdeLhs::iterator gen_end, MdeLhs::iterator spec_end) {
        // Fewer classifiers in gen, gen generalizes spec.
        if (++gen_it == gen_end) return true;
        // More classifiers in gen, gen is incomparable with spec.
        if (++spec_it == spec_end) return false;
        return IsGeneralization(gen_it, spec_it, gen_end, spec_end);
    }

    static ComparisonResult CompareLhss(MdeLhs const& cur, MdeLhs const& prev);

public:
    static constexpr bool kNeedsEmptyRemoval = true;

    void NewBatch(std::size_t elements);
    void AddGeneralizations(lattice::MdeLattice::ValidationUpdater& updater,
                            boost::dynamic_bitset<>& considered_indices);
    std::vector<ValidationSelection> GetAll() noexcept;
};
}  // namespace algos::hymde::cover_calculation
