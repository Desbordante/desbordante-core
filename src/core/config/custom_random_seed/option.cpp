#include "config/custom_random_seed/option.h"

#include <optional>  // for nullopt_t, nullopt
#include <variant>   // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "custom_random_seed/type.h"  // for CustomRandomSeedType

namespace config {
extern CommonOption<CustomRandomSeedType> const kCustomRandomFlagOpt{
        names::kCustomRandom, descriptions::kDCustomRandom, []() { return std::nullopt; }};
}  // namespace config
