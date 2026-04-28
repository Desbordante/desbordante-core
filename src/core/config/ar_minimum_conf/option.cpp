#include "core/config/ar_minimum_conf/option.h"

#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kArMinimumConfidence, descriptions::kDArMinimumConfidence;
extern CommonOption<ArMinimumConfidenceType> const kArMinimumConfidenceOpt{
        kArMinimumConfidence, kDArMinimumConfidence, 0.0, {}, [](ArMinimumConfidenceType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("Minimum confidence value should be between 0 and 1.");
            }
        }};
}  // namespace config
