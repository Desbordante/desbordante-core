#include "algorithms/mde/hymde/cover_calculation/lattice_traverser.h"

namespace algos::hymde::cover_calculation {
auto LatticeTraverser::AdjustLattice(std::vector<ValidationSelection>& validations,
                                     std::vector<BatchValidator::Result> const& results)
        -> LatticeStatistics {
    assert(validations.size() == results.size());
    LatticeStatistics lattice_statistics;
    for (auto [validation, result] : utility::Zip(validations, results)) {
        lattice_statistics.CountOne(validation, result);
        lattice::MdeLattice::ValidationUpdater& messenger = *validation.updater;
        if (validator_.Supported(result.support)) {
            messenger.LowerAndSpecialize(result.invalidated_rhss);
        } else {
            messenger.MarkUnsupported();
        }
    }
    return lattice_statistics;
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

auto LatticeTraverser::ProcessResults(std::vector<ValidationSelection>& validations,
                                      std::vector<BatchValidator::Result> const& results)
        -> LatticeStatistics {
    if (pool_ == nullptr) {
        AddRecommendations(results);
        return AdjustLattice(validations, results);
    } else {
        LatticeStatistics lattice_statistics;
        util::WorkerThreadPool::Waiter waiter = pool_->SubmitSingleTask(
                [&]() { lattice_statistics = AdjustLattice(validations, results); });
        AddRecommendations(results);
        waiter.Wait();
        return lattice_statistics;
    }
}

bool LatticeTraverser::TraverseLattice(bool const traverse_all) {
    std::vector<ValidationSelection> validations;
    while (!(validations = level_getter_.GetPendingGroupedMinimalLhsMds()).empty()) {
        std::vector<BatchValidator::Result> const& results = validator_.ValidateBatch(validations);

        LatticeStatistics lattice_statistics = ProcessResults(validations, results);

        if (!traverse_all && lattice_statistics.TraversalInefficient()) return false;
        recommendations_.clear();
    }
    return true;
}
}  // namespace algos::hymde::cover_calculation
