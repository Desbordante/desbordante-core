#include "algorithms/md/hymd/lattice_traverser.h"

#include "model/index.h"

namespace algos::hymd {

bool LatticeTraverser::TraverseLattice(bool const traverse_all) {
    using model::Index;
    while (level_getter_->AreLevelsLeft()) {
        LatticeStatistics lattice_statistics;
        std::vector<lattice::ValidationInfo> validations = level_getter_->GetCurrentMds();
        if (validations.empty()) {
            continue;
        }

        for (auto const& validation : validations) {
            lattice_statistics.all_mds_num += validation.rhs_indices.count();
        }
        std::vector<Validator::Result> const& results = validator_.ValidateAll(validations);
        auto viol_func = [this, &results]() {
            for (Validator::Result const& result : results) {
                /*if (result.is_unsupported) continue; ???*/
                for (std::vector<Recommendation> const& rhs_violations : result.recommendations) {
                    recommendations_.insert(recommendations_.end(), rhs_violations.begin(),
                                            rhs_violations.end());
                };
            }
        };
        if (pool_ == nullptr) {
            viol_func();
        } else {
            pool_->ExecSingle(viol_func);
        }
        std::size_t const validations_size = validations.size();
        for (Index i = 0; i != validations_size; ++i) {
            Validator::Result const& result = results[i];
            lattice_statistics.invalidated_mds_num += result.invalidated.Size();
            lattice::MdLattice::MdVerificationMessenger& messenger = *validations[i].messenger;
            if (result.is_unsupported) {
                messenger.MarkUnsupported();
            } else {
                messenger.LowerAndSpecialize(result.invalidated);
            }
        }
        if (pool_ != nullptr) pool_->Wait();
        if (!traverse_all &&
            lattice_statistics.invalidated_mds_num >
                    lattice_statistics.kRatioBound * (lattice_statistics.all_mds_num -
                                                      lattice_statistics.invalidated_mds_num))
            return false;
        recommendations_.clear();
    }
    return true;
}

}  // namespace algos::hymd
