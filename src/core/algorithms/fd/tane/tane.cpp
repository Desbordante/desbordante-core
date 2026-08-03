#include "core/algorithms/fd/tane/tane.h"

#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/algorithms/fd/pli_based_fd_algorithm.h"
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
        return afd_metric_calculator::AFDMetricCalculator::CalculateZeroAryG1(
                rhs, relation_.get()->GetNumTuplePairs());
    return 1;
}

config::ErrorType Tane::CalculateFdError(model::PLIWithSingletons const* lhs_pli,
                                         model::PLIWithSingletons const* rhs_pli,
                                         model::PLIWithSingletons const* joint_pli) {
    switch (afd_error_measure_) {
        case AfdErrorMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepMeasure(lhs_pli,
                                                                                        joint_pli);
        case AfdErrorMeasure::kTau:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateTau(lhs_pli, rhs_pli,
                                                                                joint_pli);
        case AfdErrorMeasure::kMuPlus:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateMuPlus(lhs_pli, rhs_pli,
                                                                                   joint_pli);
        case AfdErrorMeasure::kRho:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateRhoMeasure(lhs_pli,
                                                                                       joint_pli);
        case AfdErrorMeasure::kFi:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateFI(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case AfdErrorMeasure::kG2:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateG2(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case AfdErrorMeasure::kG3:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateG3(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        default:
            return afd_metric_calculator::AFDMetricCalculator::CalculateG1Error(
                    lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
    }
}

}  // namespace algos
