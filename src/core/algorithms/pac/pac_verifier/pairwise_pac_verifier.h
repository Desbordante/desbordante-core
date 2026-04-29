#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"

namespace algos::pac_verifier {
/// @brief Utility base class for PAC-Man-based PAC verifiers that work with pairs of tuples
class PairWisePACVerifier : public PACVerifier {
protected:
    /// @brief Get delta for a given number of pairs
    virtual double GetDelta(std::size_t num_pairs) const = 0;
    /// @brief Get number of pairs for a given delta
    /// The value shouldn't be rounded
    virtual double GetNumPairs(double delta) const = 0;
    /// @brief For each delta_i find the least eps_i such that PAC_{eps_i}^{delta_i} holds.
    /// Then refine delta_i, i. e. find the greatest delta_i' such that PAC_{eps_i}^{delta_i} holds.
    /// @return (eps_i, delta_i') pairs
    std::vector<std::pair<double, double>> CalculateEmpiricalProbabilities(
            std::vector<TuplePair> const& sorted_pairs) const;
    std::pair<double, double> GetEpsilonDeltaForEpsilonImpl(
            double epsilon, std::vector<TuplePair> const& pairs) const;
};
}  // namespace algos::pac_verifier
