#include "config/max_lhs/option.h"

#include <limits>

#include "config/names_and_descriptions.h"

namespace config {
using names::kMaximumLhs, descriptions::kDMaximumLhs;
extern CommonOption<MaxLhsType> const kMaxLhsOpt{kMaximumLhs, kDMaximumLhs,
                                                 std::numeric_limits<MaxLhsType>::max()};
}  // namespace config
