#include "tane.h"

#include "afd_measures.h"
#include "config/error/option.h"
#include "config/error_measure/option.h"
#include "enums.h"
#include "fd/afd_metric/afd_metric_calculator.h"
#include "fd/pli_based_fd_algorithm.h"
#include "model/table/column_data.h"

namespace algos {

Tane::Tane() : tane::TaneCommon() {
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

config::ErrorType Tane::CalculateFdError(model::PLIWithSingletons const* lhs_pli,
                                         model::PLIWithSingletons const* rhs_pli,
                                         model::PLIWithSingletons const* joint_pli) {
    switch (afd_error_measure_) {
        case +AfdErrorMeasure::pdep:
            return 1 - CalculatePdepMeasure(lhs_pli, joint_pli);
        case +AfdErrorMeasure::tau:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateTau(
                               lhs_pli, rhs_pli, relation_.get()->GetNumRows());
        case +AfdErrorMeasure::mu_plus:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateMuPlus(
                               lhs_pli, rhs_pli, relation_.get()->GetNumRows());
        case +AfdErrorMeasure::rho:
            return 1 - CalculateRhoMeasure(lhs_pli, joint_pli);
        default:
            return CalculateG1Error(lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
    }
}

}  // namespace algos
