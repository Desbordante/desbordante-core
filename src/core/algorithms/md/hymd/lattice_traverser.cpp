#include "algorithms/md/hymd/lattice_traverser.h"

#include <cassert>

#include "algorithms/md/hymd/utility/zip.h"
#include "md/hymd/lattice/level_getter.h"
#include "md/hymd/lattice/md_lattice.h"
#include "md/hymd/validator.h"
#include "worker_thread_pool.h"

namespace algos::hymd {
auto LatticeTraverser::AdjustLattice(std::vector<lattice::ValidationInfo>& validations,
                                     std::vector<BatchValidator::Result> const& results)
        -> LatticeStatistics {
    assert(validations.size() == results.size());
    LatticeStatistics lattice_statistics;
    for (auto [validation, result] : utility::Zip(validations, results)) {
        lattice_statistics.CountOne(validation, result);
        lattice::MdLattice::MdVerificationMessenger& messenger = *validation.messenger;
        if (result.lhs_is_unsupported) {
            messenger.MarkUnsupported();
        } else {
            messenger.LowerAndSpecialize(result.invalidated_rhss);
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

auto LatticeTraverser::ProcessResults(std::vector<lattice::ValidationInfo>& validations,
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
    std::vector<lattice::ValidationInfo> validations;
    while (!(validations = level_getter_.GetPendingGroupedMinimalLhsMds()).empty()) {
        std::vector<BatchValidator::Result> const& results = validator_.ValidateBatch(validations);

        LatticeStatistics lattice_statistics = ProcessResults(validations, results);

        if (!traverse_all && lattice_statistics.TraversalInefficient()) return false;
        recommendations_.clear();
    }
    return true;
}

}  // namespace algos::hymd
