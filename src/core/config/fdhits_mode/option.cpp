#include "core/config/fdhits_mode/option.h"

#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"

namespace config {
using names::kFDHitsMode, descriptions::kDFDHitsMode;
extern CommonOption<FDHitsModeType> const kFDHitsModeOpt{
        kFDHitsMode, kDFDHitsMode, "union", [](auto& value) {
            if (value != "union" && value != "per_target") {
                throw ConfigurationError("fdhits_mode must be one of: union, per_target");
            }
        }};
}  // namespace config
