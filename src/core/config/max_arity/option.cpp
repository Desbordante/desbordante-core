#include "config/max_arity/option.h"

#include <limits>   // for numeric_limits
#include <variant>  // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "exceptions.h"      // for ConfigurationError
#include "max_arity/type.h"  // for MaxArityType

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
