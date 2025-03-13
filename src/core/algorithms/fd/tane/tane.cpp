#include "tane.h"

#include <memory>  // for shared_ptr
#include <vector>  // for vector

#include <boost/type_index/type_index_facade.hpp>  // for operator==

#include "afd_measures.h"                       // for CalculateG1Error
#include "common_option.h"                      // for CommonOption
#include "config/error/option.h"                // for kErrorOpt
#include "config/error_measure/option.h"        // for kAfdErrorMeasureOpt
#include "enums.h"                              // for AfdErrorMeasure
#include "error/type.h"                         // for ErrorType
#include "fd/tane/tane_common.h"                // for TaneCommon
#include "table/column_layout_relation_data.h"  // for ColumnLayoutRelati...

class ColumnData;

namespace model {
class PositionListIndex;
}

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
