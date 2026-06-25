#include "core/config/use_pliws/option.h"

#include "core/config/names_and_descriptions.h"

namespace config {
using names::kUsePliws, descriptions::kDUsePliws;
extern CommonOption<bool> const kUsePliwsOpt{kUsePliws, kDUsePliws, false};
}  // namespace config
