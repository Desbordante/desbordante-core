#include "config/conditions/condition_type/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kAlgoType, descriptions::kDAlgoType;
extern CommonOption<algos::cind::AlgoType> const kAlgoTypeOpt{kAlgoType, kDAlgoType,
                                                              algos::cind::AlgoType::cinderella};
}  // namespace config
