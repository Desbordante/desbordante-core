#include "config/error_measure/option.h"

#include <variant>

#include <enum.h>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "error_measure/type.h"
#include "fd/tane/enums.h"

namespace config {
using names::kPfdErrorMeasure, names::kAfdErrorMeasure, descriptions::kDPfdErrorMeasure,
        descriptions::kDAfdErrorMeasure;
extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt{
        kPfdErrorMeasure, kDPfdErrorMeasure, algos::PfdErrorMeasure::_values()[0]};

extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt{
        kAfdErrorMeasure, kDAfdErrorMeasure, algos::AfdErrorMeasure::_values()[0]};
}  // namespace config
