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
    // NOTE: Sometimes the empty LHS case is not defined, so we have to figure out a value that
    // makes sense on our own. If the RHS is constant, there is an FD, so it only makes sense for
    // error to be 0.
    switch (afd_error_measure_) {
        case AfdErrorMeasure::kG3: {
            std::size_t max = 1;
            model::PositionListIndex const* x_pli = rhs->GetPositionListIndex();
            for (model::PLI::Cluster const& x_cluster : x_pli->GetIndex()) {
                std::size_t const x_cluster_size = x_cluster.size();
                if (max < x_cluster_size) max = x_cluster_size;
            }
            return 1.0 - static_cast<config::ErrorType>(max) / x_pli->GetRelationSize();
        }
        case AfdErrorMeasure::kG1:
            return afd_metric_calculator::AFDMetricCalculator::CalculateZeroAryG1(
                    rhs, relation_.get()->GetNumTuplePairs());
        /*
         * dom_{empty_set}(R) needs some care in its definition. If that care is taken, we get
         * |dom_{empty_set}(R)| = 1.
         */
        case AfdErrorMeasure::kRho:
            return static_cast<config::ErrorType>(rhs->GetPositionListIndex()->GetNumCluster() -
                                                  1) /
                   rhs->GetPositionListIndex()->GetNumCluster();
        /*
         * The original definition of this one requires the presence of two attributes. The exact
         * expression used is pdep(X, Y) = p(R1.Y = R.Y2 | R1.X = R2.X). If we treat the projection
         * of a tuple on empty X as the empty set, then we get pdep({}, Y) = p(R1.Y = R.Y2), which
         * is exactly the self-dependency measure pdep(Y).
         */
        case AfdErrorMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepSelf(
                               rhs->GetPositionListIndex());
        /*
         * The probability that a tuple participates in a violating pair is 0 if there is an FD,
         * otherwise it is 1 for an empty LHS and non-constant RHS.
         */
        case AfdErrorMeasure::kG2:
        /*
         * For an empty LHS, the mutual information is 0, but if the entropy of RHS is also 0 (i.e.
         * it is constant), the measure is technically undefined.
         */
        case AfdErrorMeasure::kFi:
        /*
         * When using pdep({}, Y) = pdep(Y), tau has 0 in the numerator. If pdep(Y) = 0, it has 0 in
         * the denominator too, so it is technically undefined.
         */
        case AfdErrorMeasure::kTau:
        /*
         * Since Y is taken to be constant in the definition, pdep({} -> Y, R) is just pdep(Y). The
         * expected value of a constant is that constant, so we get a 0 in the numerator. The
         * denominator is again 0 when the RHS is constant, so this measure is technically undefined
         * as well in that case.
         */
        case AfdErrorMeasure::kMuPlus:
            return rhs->GetPositionListIndex()->IsConstant() ? 0.0 : 1.0;
    }
    assert(false);
    __builtin_unreachable();
}

config::ErrorType Tane::CalculateFdError(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                         model::PLI const* joint_pli) {
    switch (afd_error_measure_) {
        case AfdErrorMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepMeasure(lhs_pli,
                                                                                        rhs_pli);
        case AfdErrorMeasure::kTau:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateTau(lhs_pli, rhs_pli);
        case AfdErrorMeasure::kMuPlus:
            return 1 -
                   afd_metric_calculator::AFDMetricCalculator::CalculateMuPlus(lhs_pli, rhs_pli);
        case AfdErrorMeasure::kRho:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateRhoMeasure(lhs_pli,
                                                                                       joint_pli);
        case AfdErrorMeasure::kFi:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateFI(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case AfdErrorMeasure::kG2:
            return afd_metric_calculator::AFDMetricCalculator::CalculateG2Error(
                    lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case AfdErrorMeasure::kG3:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateG3(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case AfdErrorMeasure::kG1:
            return afd_metric_calculator::AFDMetricCalculator::CalculateG1Error(
                    lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
    }
    assert(false);
    __builtin_unreachable();
}

}  // namespace algos
