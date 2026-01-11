#include "core/algorithms/fd/tane/tane.h"

#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/algorithms/fd/tane/afd_measures.h"
#include "core/algorithms/fd/tane/enums.h"
#include "core/config/error/option.h"
#include "core/config/error_measure/option.h"
#include "core/model/table/column_data.h"

namespace algos {

Tane::Tane() : tane::TaneCommon() {
    RegisterOption(config::kAfdErrorMeasureOpt(&afd_error_measure_));
}

void Tane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::kAfdErrorMeasureOpt.GetName()});
}

config::ErrorType Tane::CalculateZeroAryFdError(ColumnData const* rhs) {
    if (afd_error_measure_ == AfdErrorMeasure::kG1)
        return CalculateZeroAryG1(rhs, relation_.get()->GetNumTuplePairs());
    return 1;
}

config::ErrorType Tane::CalculateFdError(model::PositionListIndex const* lhs_pli,
                                         model::PositionListIndex const* rhs_pli,
                                         model::PositionListIndex const* joint_pli) {
    switch (afd_error_measure_) {
        case AfdErrorMeasure::kPdep:
            return 1 - CalculatePdepMeasure(lhs_pli, joint_pli);
        case AfdErrorMeasure::kTau:
            return 1 - CalculateTauMeasure(lhs_pli, rhs_pli, joint_pli);
        case AfdErrorMeasure::kMuPlus:
            return 1 - CalculateMuPlusMeasure(lhs_pli, rhs_pli, joint_pli);
        case AfdErrorMeasure::kRho:
            return 1 - CalculateRhoMeasure(lhs_pli, joint_pli);
        default:
            return CalculateG1Error(lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
    }
}

}  // namespace algos
