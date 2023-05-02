#include "algorithms/options/equal_nulls/option.h"

#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {
using names::kEqualNulls, descriptions::kDEqualNulls;
extern const CommonOption<EqNullsType> EqualNullsOpt{kEqualNulls, kDEqualNulls, true};
}  // namespace algos::config
