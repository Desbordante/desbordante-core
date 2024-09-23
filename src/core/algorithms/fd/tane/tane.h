#pragma once

#include "config/error/type.h"
#include "model/table/column_data.h"
#include "model/table/position_list_index.h"
#include "tane_common.h"

namespace algos {

class Tane : public tane::TaneCommon {
private:
    void MakeExecuteOptsAvailableFDInternal() override final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs) override;
    config::ErrorType CalculateFdError(model::PositionListIndex const* lhs_pli,
                                       model::PositionListIndex const* joint_pli) override;

public:
    Tane(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
};

}  // namespace algos
