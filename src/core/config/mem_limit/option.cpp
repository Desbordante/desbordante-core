#include "config/mem_limit/option.h"

#include <string>   // for to_string
#include <variant>  // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "exceptions.h"      // for ConfigurationError
#include "mem_limit/type.h"  // for MemLimitMBType

namespace config {
using names::kMemLimitMB, descriptions::kDMemLimitMB;
extern CommonOption<MemLimitMBType> const kMemLimitMbOpt{
        kMemLimitMB, kDMemLimitMB, 2 * 1024u, [](auto &value) {
            constexpr MemLimitMBType min_limit_mb = 16u;
            if (value < min_limit_mb) {
                throw ConfigurationError("Memory limit must be at least " + std::to_string(value) +
                                         "MB");
            }
        }};
}  // namespace config
