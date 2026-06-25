#pragma once

#include "core/algorithms/fd/tane/enums.h"
#include "core/algorithms/fd/tane/tane_common.h"
#include "core/config/error/type.h"
#include "core/model/table/column_data.h"
#include "core/model/table/position_list_index.h"

namespace algos {

class Tane : public tane::TaneCommon {
private:
    AfdErrorMeasure afd_error_measure_ = AfdErrorMeasure::kG1;
    void MakeExecuteOptsAvailableFDInternal() override final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs) override;
    config::ErrorType CalculateFdError(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                       model::PLI const* joint_pli) override;
    config::ErrorType CalculateFdError(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                       model::PLIWS const* joint_pli) override;

public:
    Tane();
};

}  // namespace algos
