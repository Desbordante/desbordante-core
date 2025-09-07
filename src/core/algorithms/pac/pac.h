#pragma once

#include <string>

namespace model {

/// @brief Probability Approximate Constraints are dependencies that generalize exact dependencies.
/// Each PAC has two common parameters: epsilon and delta.
/// Epsilon is a measure of "proximity", which shows how much PAC deviates from exact dependency.
/// Delta is a probability at which approximate dependency holds.
class PAC {
protected:
    double epsilon_;
    double delta_;

public:
    virtual ~PAC() = default;

    PAC(double epsilon, double delta) : epsilon_(epsilon), delta_(delta) {}

    virtual std::string ToShortString() const = 0;
    virtual std::string ToLongString() const = 0;

    double GetEpsilon() const {
        return epsilon_;
    }

    void SetEpsilon(double const new_value) {
        epsilon_ = new_value;
    }

    double GetDelta() const {
        return delta_;
    }

    void SetDelta(double const new_value) {
        delta_ = new_value;
    }
};
}  // namespace model
