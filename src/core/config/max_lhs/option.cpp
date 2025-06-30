#include "config/max_lhs/option.h"

#include <limits>   // for numeric_limits
#include <variant>  // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "max_lhs/type.h"  // for MaxLhsType

namespace config {
using names::kMaximumLhs, descriptions::kDMaximumLhs;
extern CommonOption<MaxLhsType> const kMaxLhsOpt{kMaximumLhs, kDMaximumLhs,
                                                 std::numeric_limits<MaxLhsType>::max()};
}  // namespace config
