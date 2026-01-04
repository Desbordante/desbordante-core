#include "core/config/conditions/condition_type/option.h"

#include "core/config/names_and_descriptions.h"

namespace config {
using names::kConditionType, descriptions::kDConditionType;
extern CommonOption<algos::cind::CondType> const kConditionTypeOpt{
        kConditionType, kDConditionType, algos::cind::CondType::row};
}  // namespace config
