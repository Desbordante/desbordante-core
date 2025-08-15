#include "algorithms/mde/hymde/cover_calculation/lattice_traverser.h"

namespace algos::hymde::cover_calculation {
void LatticeTraverser::AdjustLattice(LatticeStatistics& statistics,
                                     std::vector<ValidationSelection>& validations,
                                     std::vector<BatchValidator::Result> const& results) {
    assert(validations.size() == results.size());
    for (auto [validation, result] : utility::Zip(validations, results)) {
        statistics.AddInvalidatedNumber(result);
        lattice::MdeLattice::ValidationUpdater& messenger = *validation.updater;
        if (validator_.Supported(result.support)) {
            messenger.LowerAndSpecialize(result.invalidated_rhss);
        } else {
            messenger.MarkUnsupported();
        }
    }
}

bool LatticeTraverser::TraverseLattice(bool const traverse_all) {
    std::vector<ValidationSelection> selections;
    while (!(selections = level_getter_.GetPendingGroupedMinimalLhsMds()).empty()) {
        LatticeStatistics lattice_statistics;
        lattice_statistics.CountMdsAssumedPartOfMinimalCoverOfValid(selections);

        std::vector<BatchValidator::Result> const& results = validator_.ValidateBatch(selections);

        AdjustLattice(lattice_statistics, selections, results);

        if (!traverse_all && lattice_statistics.TraversalInefficient()) return false;
    }
    return true;
}
}  // namespace algos::hymde::cover_calculation
