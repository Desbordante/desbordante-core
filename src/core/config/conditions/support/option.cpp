#include "core/config/conditions/support/option.h"

#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kCindMinSupport, descriptions::kDCindMinSupport;
extern CommonOption<unsigned int> const kCindMinSupportOpt{
        kCindMinSupport, kDCindMinSupport, 2u, {}, [](unsigned int support) {
            if (support < 1) {
                throw ConfigurationError("ERROR: support must be >= 1.");
            }
        }};
}  // namespace config
