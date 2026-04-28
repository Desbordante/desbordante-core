#include "core/config/error_measure/option.h"

#include <magic_enum/magic_enum.hpp>

#include "core/algorithms/fd/tane/enums.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kPfdErrorMeasure, names::kAfdErrorMeasure, descriptions::kDPfdErrorMeasure,
        descriptions::kDAfdErrorMeasure;
extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt{
        kPfdErrorMeasure, kDPfdErrorMeasure,
        magic_enum::enum_values<algos::PfdErrorMeasure>().front()};

extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt{
        kAfdErrorMeasure, kDAfdErrorMeasure,
        magic_enum::enum_values<algos::AfdErrorMeasure>().front()};
}  // namespace config
