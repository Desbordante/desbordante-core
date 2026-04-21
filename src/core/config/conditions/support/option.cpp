#include "core/config/conditions/support/option.h"

#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kSupport, descriptions::kDSupport;
extern CommonOption<unsigned int> const kSupportOpt{kSupport, kDSupport, 2u, {},
                                                    [](unsigned int support) {
                                                        if (support < 1) {
                                                            throw ConfigurationError(
                                                                    "ERROR: support must be >= 1.");
                                                        }
                                                    }};
}  // namespace config
