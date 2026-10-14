#include "core/config/error_measure/option.h"

#include "core/algorithms/fd/tane/enums.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kPfdErrorMeasure, names::kAfdErrorMeasure, descriptions::kDPfdErrorMeasure,
        descriptions::kDAfdErrorMeasure;
extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt{
        kPfdErrorMeasure, kDPfdErrorMeasure, PfdErrorMeasureType::kPerTuple};

extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt{
        kAfdErrorMeasure, kDAfdErrorMeasure, AfdErrorMeasureType::kG1};
}  // namespace config
