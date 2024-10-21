#include "tane.h"

#include "afd_measures.h"
#include "config/error/option.h"
#include "config/error_measure/option.h"
#include "enums.h"
#include "fd/pli_based_fd_algorithm.h"
#include "model/table/column_data.h"

namespace algos {

Tane::Tane(std::optional<ColumnLayoutRelationDataManager> relation_manager)
    : tane::TaneCommon(relation_manager) {
    RegisterOption(config::kAfdErrorMeasureOpt(&afd_error_measure_));
}

void Tane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::kAfdErrorMeasureOpt.GetName()});
}

config::ErrorType Tane::CalculateZeroAryFdError(ColumnData const* rhs) {
    if (afd_error_measure_ == +AfdErrorMeasure::g1)
        return CalculateZeroAryG1(rhs, relation_.get()->GetNumTuplePairs());
    return 1;
}

config::ErrorType Tane::CalculateFdError(model::PositionListIndex const* lhs_pli,
                                         model::PositionListIndex const* rhs_pli,
                                         model::PositionListIndex const* joint_pli) {
    switch (afd_error_measure_) {
        case +AfdErrorMeasure::pdep:
            return 1 - CalculatePdepMeasure(lhs_pli, joint_pli);
        case +AfdErrorMeasure::tau:
            return 1 - CalculateTauMeasure(lhs_pli, rhs_pli, joint_pli);
        case +AfdErrorMeasure::mu_plus:
            return 1 - CalculateMuPlusMeasure(lhs_pli, rhs_pli, joint_pli);
        case +AfdErrorMeasure::rho:
            return 1 - CalculateRhoMeasure(lhs_pli, joint_pli);
        default:
            return CalculateG1Error(lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
    }
}

}  // namespace algos
