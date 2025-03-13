#include "config/error/option.h"

#include <variant>  // for variant

#include "common_option.h"      // for CommonOption
#include "config/exceptions.h"  // for ConfigurationError
#include "config/names_and_descriptions.h"
#include "error/type.h"  // for ErrorType

namespace config {
using names::kError, descriptions::kDError;
extern CommonOption<ErrorType> const kErrorOpt{
        kError, kDError, 0.0, {}, [](ErrorType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("ERROR: error should be between 0 and 1.");
            }
        }};
}  // namespace config
