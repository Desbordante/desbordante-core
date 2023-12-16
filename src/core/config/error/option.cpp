#include "config/error/option.h"

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {
using names::kError, descriptions::kDError;
extern CommonOption<ErrorType> const ErrorOpt{
        kError, kDError, 0.0, {}, [](ErrorType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("ERROR: error should be between 0 and 1.");
            }
        }};
}  // namespace config
