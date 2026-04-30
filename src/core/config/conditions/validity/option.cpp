#include "core/config/conditions/validity/option.h"

#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kValidity, descriptions::kDValidity;
extern CommonOption<ErrorType> const kValidityOpt{
        kValidity, kDValidity, 1.0, {}, [](ErrorType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("ERROR: completeness should be between 0 and 1.");
            }
        }};
}  // namespace config
