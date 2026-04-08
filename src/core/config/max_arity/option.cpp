#include "core/config/max_arity/option.h"

#include <limits>
#include <optional>
#include <string>
#include <variant>

#include "core/config/names_and_descriptions.h"
#include "core/config/exceptions.h"

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
