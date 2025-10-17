#include "config/custom_random_seed/option.h"

#include <optional>
#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "custom_random_seed/type.h"
#include "descriptions.h"
#include "names.h"
#include "names_and_descriptions.h"

namespace config {
extern CommonOption<CustomRandomSeedType> const kCustomRandomFlagOpt{
        names::kCustomRandom, descriptions::kDCustomRandom, []() { return std::nullopt; }};
}  // namespace config
