#include "config/conditions/completeness/option.h"

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {
using names::kCompleteness, descriptions::kDCompleteness;
extern CommonOption<ErrorType> const kCompletenessOpt{
        kCompleteness, kDCompleteness, 0.0, {}, [](ErrorType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("ERROR: completeness should be between 0 and 1.");
            }
        }};
}  // namespace config
