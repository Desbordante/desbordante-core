#pragma once

#include <string>

#include "algorithms/mde/decision_boundaries/value.h"

namespace model::mde::decision_boundaries {
template <typename Type>
class StandardStringValue : public Value<Type> {
public:
    using Value<Type>::Value;

    std::string ToString() const final {
        return std::to_string(Value<Type>::value_);
    }
};
}  // namespace model::mde::decision_boundaries
