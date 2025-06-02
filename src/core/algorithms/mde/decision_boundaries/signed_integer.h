#pragma once

#include <cstdint>

#include "algorithms/mde/decision_boundaries/standard_string_value.h"

namespace model::mde::decision_boundaries {
class SignedInteger : public StandardStringValue<std::intmax_t> {
    using StandardStringValue<std::intmax_t>::StandardStringValue;
};
}  // namespace model::mde::decision_boundaries
