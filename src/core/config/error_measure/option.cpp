#include "config/error_measure/option.h"

#include <variant>  // for variant

#include <enum.h>  // for _iterable

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "error_measure/type.h"  // for AfdErrorMeasureType, PfdErrorMeasure...
#include "fd/tane/enums.h"       // for AfdErrorMeasure, PfdErrorMeasure

namespace config {
using names::kPfdErrorMeasure, names::kAfdErrorMeasure, descriptions::kDPfdErrorMeasure,
        descriptions::kDAfdErrorMeasure;
extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt{
        kPfdErrorMeasure, kDPfdErrorMeasure, algos::PfdErrorMeasure::_values()[0]};

extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt{
        kAfdErrorMeasure, kDAfdErrorMeasure, algos::AfdErrorMeasure::_values()[0]};
}  // namespace config
