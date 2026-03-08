
#include "core/algorithms/fd/tane/enums.h"
#include "core/config/error/type.h"
#include "core/model/table/column_data.h"
#include "core/model/table/position_list_index.h"

namespace algos {
config::ErrorType CalculateZeroAryG1(ColumnData const* rhs, unsigned long long num_tuple_pairs);

config::ErrorType CalculateG1Error(model::PLIWS const* lhs_pli, model::PLIWS const* joint_pli,
                                   unsigned long long num_tuple_pairs);

config::ErrorType PdepSelf(model::PLI const* x_pli);

config::ErrorType CalculatePdepMeasure(model::PLI const* x_pli, model::PLI const* xa_pli);

config::ErrorType CalculateTauMeasure(model::PLIWS const* x_pli, model::PLIWS const* a_pli,
                                      model::PLIWS const* xa_pli);

config::ErrorType CalculateMuPlusMeasure(model::PLIWS const* x_pli, model::PLIWS const* a_pli,
                                         model::PLIWS const* xa_pli);

config::ErrorType CalculateRhoMeasure(model::PLIWS const* x_pli, model::PLIWS const* xa_pli);
}  // namespace algos
