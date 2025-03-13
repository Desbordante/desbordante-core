#pragma once

#include <optional>  // for optional, nullopt, nullopt_t

#include "config/error/type.h"  // for ErrorType
#include "enums.h"              // for operator+, AfdErrorMeasure
#include "tane_common.h"        // for TaneCommon

class ColumnData;

namespace model {
class PositionListIndex;
}

namespace algos {

class Tane : public tane::TaneCommon {
private:
    AfdErrorMeasure afd_error_measure_ = +AfdErrorMeasure::g1;
    void MakeExecuteOptsAvailableFDInternal() override final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs) override;
    config::ErrorType CalculateFdError(model::PositionListIndex const* lhs_pli,
                                       model::PositionListIndex const* rhs_pli,
                                       model::PositionListIndex const* joint_pli) override;

public:
    Tane(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
};

}  // namespace algos
