#pragma once

#include <cstdint>

namespace model {

using Event = uint64_t;

constexpr Event kInvalidEvent = -1;
constexpr Event kStartEvent = 0;

}  // namespace model
