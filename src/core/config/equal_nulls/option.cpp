#include "config/equal_nulls/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kEqualNulls, descriptions::kDEqualNulls;
extern const CommonOption<EqNullsType> EqualNullsOpt{kEqualNulls, kDEqualNulls, true};
}  // namespace config
