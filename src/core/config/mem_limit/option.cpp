#include "config/mem_limit/option.h"

#include <string>
#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "descriptions.h"
#include "exceptions.h"
#include "mem_limit/type.h"
#include "names.h"
#include "names_and_descriptions.h"

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
