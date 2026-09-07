#include "core/config/error_measure/option.h"

#include "core/algorithms/fd/tane/enums.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kPfdErrorMeasure, descriptions::kDPfdErrorMeasure;
extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt{
        kPfdErrorMeasure, kDPfdErrorMeasure, PfdErrorMeasureType::kPerTuple};
}  // namespace config
