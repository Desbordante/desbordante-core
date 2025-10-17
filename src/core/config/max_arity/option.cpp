#include "config/max_arity/option.h"

#include <limits>
#include <string>
#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "descriptions.h"
#include "exceptions.h"
#include "max_arity/type.h"
#include "names.h"
#include "names_and_descriptions.h"

namespace config {
using names::kMaximumArity, descriptions::kDMaximumArity;
extern CommonOption<MaxArityType> const kMaxArityOpt{
        kMaximumArity, kDMaximumArity, std::numeric_limits<MaxArityType>::max(), nullptr,
        [](MaxArityType max_arity) {
            if (max_arity == 0) {
                throw ConfigurationError(
                        "Mining dependencies with maximum arity 0 is meaningless.");
            }
        }

};
}  // namespace config
