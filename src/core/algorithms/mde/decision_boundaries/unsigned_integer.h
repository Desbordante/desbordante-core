#pragma once

#include <cstdint>

#include "algorithms/mde/decision_boundaries/standard_string_value.h"

namespace model::mde::decision_boundaries {
class UnsignedInteger : public StandardStringValue<std::uintmax_t> {
    using StandardStringValue<std::uintmax_t>::StandardStringValue;
};
}  // namespace model::mde::decision_boundaries


