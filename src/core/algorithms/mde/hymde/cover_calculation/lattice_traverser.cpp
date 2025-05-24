#include "algorithms/mde/hymde/cover_calculation/lattice_traverser.h"

namespace algos::hymde::cover_calculation {
void LatticeTraverser::AdjustLattice(LatticeStatistics& statistics,
                                     std::vector<ValidationSelection>& validations,
                                     std::vector<BatchValidator::Result> const& results) {
    assert(validations.size() == results.size());
    for (auto [validation, result] : utility::Zip(validations, results)) {
        statistics.CountOne(result);
        lattice::MdeLattice::ValidationUpdater& messenger = *validation.updater;
        if (validator_.Supported(result.support)) {
            messenger.LowerAndSpecialize(result.invalidated_rhss);
        } else {
            messenger.MarkUnsupported();
        }
    }
}

void LatticeTraverser::AddRecommendations(std::vector<BatchValidator::Result> const& results) {
    for (BatchValidator::Result const& result : results) {
        // TODO: decide
        /*if (result.is_unsupported) continue; ???*/
        for (BatchValidator::OneRhsRecommendations const& rhs_recommendations :
             result.all_rhs_recommendations) {
            // TODO: Just move it?
            recommendations_.insert(recommendations_.end(), rhs_recommendations.begin(),
                                    rhs_recommendations.end());
        }
    }
}

void LatticeTraverser::ProcessResults(LatticeStatistics& statistics,
                                      std::vector<ValidationSelection>& validations,
                                      std::vector<BatchValidator::Result> const& results) {
    if (pool_ == nullptr) {
        AddRecommendations(results);
        AdjustLattice(statistics, validations, results);
    } else {
        util::WorkerThreadPool::Waiter waiter =
                pool_->SubmitSingleTask([&]() { AdjustLattice(statistics, validations, results); });
        AddRecommendations(results);
        waiter.Wait();
    }
}

bool LatticeTraverser::TraverseLattice(bool const traverse_all) {
    std::vector<ValidationSelection> validations;
    while (!(validations = level_getter_.GetPendingGroupedMinimalLhsMds()).empty()) {
        LatticeStatistics lattice_statistics;
        lattice_statistics.AddExaminedMds(validations);

        std::vector<BatchValidator::Result> const& results = validator_.ValidateBatch(validations);

        // TODO: count MDs for validation before validations take place.
        ProcessResults(lattice_statistics, validations, results);

        if (!traverse_all && lattice_statistics.TraversalInefficient()) return false;
        recommendations_.clear();
    }
    return true;
}
}  // namespace algos::hymde::cover_calculation
