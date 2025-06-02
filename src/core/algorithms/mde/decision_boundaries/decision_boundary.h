#pragma once

#include <string>

namespace model::mde::decision_boundaries {
class DecisionBoundary {
public:
    virtual std::string ToString() const = 0;

    virtual ~DecisionBoundary() = default;
};
}  // namespace model::mde::decision_boundaries
