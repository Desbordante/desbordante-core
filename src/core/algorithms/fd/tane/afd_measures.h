
#include "config/error/type.h"
#include "enums.h"
#include "model/table/column_data.h"
#include "model/table/position_list_index.h"

namespace algos {
config::ErrorType CalculateZeroAryG1(ColumnData const* rhs, unsigned long long num_tuple_pairs);

config::ErrorType CalculateG1Error(model::PositionListIndex const* lhs_pli,
                                   model::PositionListIndex const* joint_pli,
                                   unsigned long long num_tuple_pairs);

config::ErrorType PdepSelf(model::PositionListIndex const* x_pli);

config::ErrorType CalculatePdepMeasure(model::PositionListIndex const* x_pli,
                                       model::PositionListIndex const* xa_pli);

config::ErrorType CalculateTauMeasure(model::PositionListIndex const* x_pli,
                                      model::PositionListIndex const* a_pli,
                                      model::PositionListIndex const* xa_pli);

config::ErrorType CalculateMuPlusMeasure(model::PositionListIndex const* x_pli,
                                         model::PositionListIndex const* a_pli,
                                         model::PositionListIndex const* xa_pli);

config::ErrorType CalculateRhoMeasure(model::PositionListIndex const* x_pli,
                                      model::PositionListIndex const* xa_pli);
}  // namespace algos
