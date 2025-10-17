#include "config/equal_nulls/option.h"

#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "descriptions.h"
#include "equal_nulls/type.h"
#include "names.h"
#include "names_and_descriptions.h"

namespace config {
using names::kEqualNulls, descriptions::kDEqualNulls;
extern CommonOption<EqNullsType> const kEqualNullsOpt{kEqualNulls, kDEqualNulls, true};
}  // namespace config
