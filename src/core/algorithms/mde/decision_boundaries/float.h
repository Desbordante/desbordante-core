#pragma once

#include <string>

#include "algorithms/mde/decision_boundaries/standard_string_value.h"

namespace model::mde::decision_boundaries {
// A float value.
class Float : public StandardStringValue<double> {
    using StandardStringValue<double>::StandardStringValue;
};
}  // namespace model::mde::decision_boundaries
