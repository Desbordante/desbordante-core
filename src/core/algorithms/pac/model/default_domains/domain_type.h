#pragma once

#include <enum.h>

namespace pac::model {
/// @brief Default domain names; used in CLI.
/// "custom" here is UntypedDomain.
BETTER_ENUM(DomainType, unsigned char, ball, parallelepiped, custom_domain);
}  // namespace pac::model
