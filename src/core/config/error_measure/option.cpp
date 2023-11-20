#include "config/error_measure/option.h"

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {
using names::kErrorMeasure, descriptions::kDErrorMeasure;
extern const CommonOption<ErrorMeasureType> ErrorMeasureOpt{
        kErrorMeasure, kDErrorMeasure, "e", {}, [](ErrorMeasureType error) {
            if (!(error == "e" || error == "per_value")) {
                throw ConfigurationError("ERROR: error function can be e or per_value");
            }
        }};
}  // namespace config
