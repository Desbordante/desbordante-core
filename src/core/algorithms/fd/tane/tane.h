#pragma once

#include "config/error/type.h"
#include "enums.h"
#include "model/table/column_data.h"
#include "model/table/position_list_index.h"
#include "tane_common.h"

namespace algos {

class Tane : public tane::TaneCommon {
private:
    ErrorMeasure error_measure_ = +ErrorMeasure::g1;
    void MakeExecuteOptsAvailableFDInternal() override final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs) override;
    config::ErrorType CalculateFdError(model::PositionListIndex const* lhs_pli,
                                       model::PositionListIndex const* rhs_pli,
                                       model::PositionListIndex const* joint_pli) override;

public:
    Tane(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
    config::ErrorType CalculateZeroAryG1(ColumnData const* rhs);
    config::ErrorType CalculateG1Error(model::PositionListIndex const* lhs_pli,
                                       model::PositionListIndex const* joint_pli);

    static config::ErrorType PdepSelf(model::PositionListIndex const* x_pli);
    static config::ErrorType CalculatePdepMeasure(model::PositionListIndex const* x_pli,
                                                  model::PositionListIndex const* xa_pli);
    static config::ErrorType CalculateTauMeasure(model::PositionListIndex const* x_pli,
                                                 model::PositionListIndex const* a_pli,
                                                 model::PositionListIndex const* xa_pli);
    static config::ErrorType CalculateMuPlusMeasure(model::PositionListIndex const* x_pli,
                                                    model::PositionListIndex const* a_pli,
                                                    model::PositionListIndex const* xa_pli);
    static config::ErrorType CalculateRhoMeasure(model::PositionListIndex const* x_pli,
                                                 model::PositionListIndex const* xa_pli);
};

}  // namespace algos
