#pragma once

#include <string>

#include "algorithms/mde/decision_boundaries/value.h"

namespace model::mde::decision_boundaries {
class Bool final : public Value<bool> {
public:
    using Value<bool>::Value;

    std::string ToString() const final {
        return value_ ? "true" : "false";
    }
};
}  // namespace model::mde::decision_boundaries
