#include "util/config/equal_nulls/option.h"

#include "util/config/names_and_descriptions.h"

namespace util::config {
using names::kEqualNulls, descriptions::kDEqualNulls;
extern const CommonOption<EqNullsType> EqualNullsOpt{kEqualNulls, kDEqualNulls, true};
}  // namespace util::config
