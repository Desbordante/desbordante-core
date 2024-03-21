#include "config/equal_nulls/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kEqualNulls, descriptions::kDEqualNulls;
extern CommonOption<EqNullsType> const kEqualNullsOpt{kEqualNulls, kDEqualNulls, true};
}  // namespace config
