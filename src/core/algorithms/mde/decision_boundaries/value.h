#pragma once

#include <string>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"

namespace model::mde::decision_boundaries {
template <typename Type>
class Value : public DecisionBoundary {
public:
    using ValueType = Type;

protected:
    ValueType value_;

public:
    Value(ValueType value) : value_(value) {}

    ValueType GetValue() {
        return value_;
    }
};
}  // namespace model::mde::decision_boundaries
