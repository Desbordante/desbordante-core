#include "config/ar_data/threshold/option.h"

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {
using names::kMinimumSupport, names::kMinimumConfidence, descriptions::kDMinimumSupport,
        descriptions::kDMinimumConfidence;
extern CommonOption<ArThresholdType> const kMinimumSupportOpt{
        kMinimumSupport, kDMinimumSupport, 0.0, {}, [](ArThresholdType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("Minimum support value should be between 0 and 1.");
            }
        }};
extern CommonOption<ArThresholdType> const kMinimumConfidenceOpt{
        kMinimumConfidence, kDMinimumConfidence, 0.0, {}, [](ArThresholdType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("Minimum confidence value should be between 0 and 1.");
            }
        }};
}  // namespace config
