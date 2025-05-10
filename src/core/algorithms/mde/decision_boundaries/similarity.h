#pragma once

#include <string>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"

namespace model::mde::decision_boundaries {
class Similarity final : public DecisionBoundary {
    double sim_;

public:
    Similarity(double sim) : sim_(sim) {}

    std::string ToString() const final {
        return std::to_string(sim_);
    }

    double GetValue() {
        return sim_;
    }
};
}  // namespace model::mde::decision_boundaries
