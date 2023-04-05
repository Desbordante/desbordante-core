#include "algorithms/options/max_lhs/option.h"

#include <limits>

#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {
using names::kMaximumLhs, descriptions::kDMaximumLhs;
extern const CommonOption<MaxLhsType> MaxLhsOpt{kMaximumLhs, kDMaximumLhs,
                                                std::numeric_limits<MaxLhsType>::max()};
}  // namespace algos::config
