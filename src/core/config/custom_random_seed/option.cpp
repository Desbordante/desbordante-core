#include "core/config/custom_random_seed/option.h"

#include <utility>

#include "core/config/names_and_descriptions.h"

namespace config {
extern CommonOption<CustomRandomSeedType> const kCustomRandomFlagOpt{
        names::kCustomRandom, descriptions::kDCustomRandom, []() { return std::nullopt; }};
}  // namespace config
