#include "config/equal_nulls/option.h"

#include <variant>  // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "equal_nulls/type.h"  // for EqNullsType

namespace config {
using names::kEqualNulls, descriptions::kDEqualNulls;
extern CommonOption<EqNullsType> const kEqualNullsOpt{kEqualNulls, kDEqualNulls, true};
}  // namespace config
