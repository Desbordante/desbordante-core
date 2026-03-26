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
    using Pairs = std::vector<TuplePair>;
    using PairsIt = Pairs::const_iterator;

    /// @brief Get delta for a given number of pairs
    virtual double GetDelta(std::size_t num_pairs) const = 0;
    /// @brief Get number of pairs for a given delta
    /// The value shouldn't be rounded
    virtual double GetNumPairs(double delta) const = 0;
    /// @brief Given a (eps, delta) pair, get (eps, delta') pair, that is better in some way
    /// Different PAC verifiers may have different refinement tactics
    /// Must produce a dereferenceable iterator (i. e. may not produce end())
    virtual void RefineDelta(PairsIt&) const = 0;
    /// @brief For each delta_i find the least eps_i such that PAC_{eps_i}^{delta_i} holds.
    /// Then refine delta_i, i. e. find the greatest delta_i' such that PAC_{eps_i}^{delta_i} holds.
    /// @return (eps_i, delta_i') pairs
    std::vector<std::pair<double, double>> CalculateEmpiricalProbabilities(
            Pairs const& sorted_pairs) const;
    std::pair<double, double> GetEpsilonDeltaForEpsilonImpl(double epsilon,
                                                            Pairs const& pairs) const;
};
}  // namespace algos::pac_verifier
