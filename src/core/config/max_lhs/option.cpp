#include "config/max_lhs/option.h"

#include <limits>
#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "descriptions.h"
#include "max_lhs/type.h"
#include "names.h"
#include "names_and_descriptions.h"

namespace config {
using names::kMaximumLhs, descriptions::kDMaximumLhs;
extern CommonOption<MaxLhsType> const kMaxLhsOpt{kMaximumLhs, kDMaximumLhs,
                                                 std::numeric_limits<MaxLhsType>::max()};
}  // namespace config
