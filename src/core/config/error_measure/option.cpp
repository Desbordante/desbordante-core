#include "core/config/error_measure/option.h"

#include "core/algorithms/fd/tane/enums.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kPfdErrorMeasure, names::kAfdErrorMeasure, descriptions::kDPfdErrorMeasure,
        descriptions::kDAfdErrorMeasure;
extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt{
        kPfdErrorMeasure, kDPfdErrorMeasure, algos::PfdErrorMeasure::_values()[0]};

extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt{
        kAfdErrorMeasure, kDAfdErrorMeasure, algos::AfdErrorMeasure::_values()[0]};
}  // namespace config
