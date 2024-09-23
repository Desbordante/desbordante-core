#include "tane.h"

#include "config/error/option.h"
#include "fd/pli_based_fd_algorithm.h"
#include "model/table/column_data.h"

namespace algos {

Tane::Tane(std::optional<ColumnLayoutRelationDataManager> relation_manager)
    : tane::TaneCommon(relation_manager) {}

void Tane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName()});
}

config::ErrorType Tane::CalculateZeroAryFdError(ColumnData const* rhs) {
    return 1 - rhs->GetPositionListIndex()->GetNepAsLong() /
                       static_cast<double>(relation_.get()->GetNumTuplePairs());
}

config::ErrorType Tane::CalculateFdError(model::PositionListIndex const* lhs_pli,
                                         model::PositionListIndex const* joint_pli) {
    return (lhs_pli->GetNepAsLong() - joint_pli->GetNepAsLong()) /
           static_cast<double>(relation_.get()->GetNumTuplePairs());
}

}  // namespace algos
