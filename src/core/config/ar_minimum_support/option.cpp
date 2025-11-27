#include "option.h"

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {
using names::kArMinimumSupport, descriptions::kDArMinimumSupport;
extern CommonOption<ArMinimumSupportType> const kArMinimumSupportOpt{
        kArMinimumSupport, kDArMinimumSupport, 0.0, {}, [](ArMinimumSupportType error) {
            if (!(error >= 0 && error <= 1)) {
                throw ConfigurationError("Minimum support value should be between 0 and 1.");
            }
        }};
}  // namespace config
