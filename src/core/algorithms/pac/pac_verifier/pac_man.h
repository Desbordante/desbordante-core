#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "core/algorithms/pac/pac_verifier/util/pac_verifier_utils.h"
#include "core/algorithms/pac/pac_verifier/util/validation_mode.h"

namespace algos::pac_verifier {
/// @brief PAC-Man algorithm. Uses elbow method to find an optimal PAC on a ECDF
class PACMan {
private:
    double min_epsilon_;
    double max_epsilon_;
    double min_delta_;
    double max_delta_;
    double diagonal_threshold_;

    /// @brief Get (refined) epsilon-delta pair with specific epsilon
    /// Must return the minimal possible epsilon when the requested @c epsilon is too small to get
    /// epsilon-delta for it
    std::function<EpsilonDelta(double)> get_epsilon_delta_for_epsilon_;

    /// @brief Remove extra pairs from empirical_probabilities
    std::ranges::subrange<std::vector<EpsilonDelta>::const_iterator> BuildECDF(
            std::vector<EpsilonDelta>& empirical_probabilities) const;

    /// @brief Check if min/max epsilon and min/max delta conditions can be satisfied simultaneously
    void CheckEpsDeltaConditions(std::vector<EpsilonDelta> const& empirical_probabilities) const;

    /// @brief Check if there are pairs between min epsilon and max epsilon, and select proper (eps,
    /// delta) if needed
    std::optional<EpsilonDelta> CheckPairsBetweenMinMaxEpsilon(
            std::vector<EpsilonDelta> const& empirical_probabilities) const;

    // Trivial getters intended to be used with STL algorithms
    static double GetEpsilon(EpsilonDelta const& eps_delta) {
        return eps_delta.epsilon;
    }

    static double GetDelta(EpsilonDelta const& eps_delta) {
        return eps_delta.delta;
    }

public:
    PACMan(double min_epsilon, double max_epsilon, double min_delta, double max_delta,
           double diagonal_threshold,
           std::function<EpsilonDelta(double)> get_epsilon_delta_for_epsilon)
        : min_epsilon_(min_epsilon),
          max_epsilon_(max_epsilon),
          min_delta_(min_delta),
          max_delta_(max_delta),
          diagonal_threshold_(diagonal_threshold),
          get_epsilon_delta_for_epsilon_(get_epsilon_delta_for_epsilon) {}

    /// @brief Find PAC's epsilon and delta using elbow method on ECDF defined by @c
    /// empirical_probabilities (epsilon-delta pairs)
    /// @return epsilon, delta
    EpsilonDelta FindEpsilonDelta(std::vector<EpsilonDelta>&& empirical_probabilities) const;
};
}  // namespace algos::pac_verifier
