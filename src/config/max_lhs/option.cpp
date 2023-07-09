#include "config/max_lhs/option.h"

#include <limits>

#include "config/names_and_descriptions.h"

namespace util::config {
using names::kMaximumLhs, descriptions::kDMaximumLhs;
extern const CommonOption<MaxLhsType> MaxLhsOpt{kMaximumLhs, kDMaximumLhs,
                                                std::numeric_limits<MaxLhsType>::max()};
}  // namespace util::config
