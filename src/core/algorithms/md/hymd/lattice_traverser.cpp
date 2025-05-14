#include "algorithms/md/hymd/lattice_traverser.h"

#include <ranges>

#include "algorithms/md/hymd/utility/zip.h"
#include "model/index.h"

namespace algos::hymd {
void LatticeTraverser::AdjustLattice(LatticeStatistics& statistics,
                                     std::vector<lattice::ValidationInfo>& validations,
                                     std::vector<BatchValidator::Result> const& results) {
    assert(validations.size() == results.size());
    for (auto [validation, result] : utility::Zip(validations, results)) {
        statistics.CountOne(result);
        lattice::MdLattice::MdVerificationMessenger& messenger = *validation.messenger;
        if (result.lhs_is_unsupported) {
            messenger.MarkUnsupported();
        } else {
            messenger.LowerAndSpecialize(result.invalidated_rhss);
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
                                      std::vector<lattice::ValidationInfo>& validations,
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
    std::vector<lattice::ValidationInfo> validations;
    while (!(validations = level_getter_.GetPendingGroupedMinimalLhsMds()).empty()) {
        LatticeStatistics lattice_statistics;
        lattice_statistics.AddExaminedMds(validations);

        std::vector<BatchValidator::Result> const& results = validator_.ValidateBatch(validations);

        ProcessResults(lattice_statistics, validations, results);

        if (!traverse_all && lattice_statistics.TraversalInefficient()) return false;
        recommendations_.clear();
    }
    return true;
}

}  // namespace algos::hymd
