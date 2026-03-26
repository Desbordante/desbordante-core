#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/algorithms/pac/fd_pac.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/pairwise_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/config/custom_metric/custom_metric.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/model/types/type.h"

namespace algos::pac_verifier {
// FD PAC X -> Y specifies that
//  if |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X
//  then Pr(|t_i[B_l] - t_j[B_l]| <= eps_l) >= delta for each B_l in Y
//
// NOTE: PAC-Man based algorithms (including this one) can handle only PACs that have
// eps_1 = ... = eps_n = eps
//
// Key steps of the algorithm:
// 1. Select subset Gamma of r^2:
//     Gamma = {(t_i, t_j) : |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X}
// 3. Sort Gamma by max|t_i[B_l] - t_j[B_l]|
// 4. For each gamma(n) find the greatest eps(n), such that epm_prob(n) <= delta(n), where
//     sigma(n) = {(t_i, t_j) in Gamma : |t_i[B_l] - t_j[B_l]| <= eps(n) for each B_l in Y}
//     emp_prob(n) = |sigma(n)| / |Gamma|
//    This step is done by repeatedly "widening" sigma(n). See Domain PAC verifier for a more
//    straightforward usage of this approach
//
// Optimizations and other notes:
// 1. For each pair (t_i, t_j) in Gamma select l0, l1 such that
//     |t_i[A_l0] - t_j[A_l0]| = max|t_i[A_l] - t_j[A_l]| for each A_l in X
//     |t_i[B_l1] - t_j[B_l1]| = max|t_i[B_l] - t_j[B_l]| for each B_l in Y
//    It's obvious that only A_l0 and B_l1 must be considered.
// 2. Only subset Gamma' of Gamma needs to be considered:
//     Gamma' = {(t_i, t_j) in Gamma : i < j},  Gamma'' = {(t_i, t_j) in Gamma : i > j}
//     sigma' = sigma \cap Gamma',  sigma'' = sigma \cap Gamma''
//    Consider a subset D of r^2: D = {(t_i, t_i) in r^2} It's obvious that
//     |D| = |r|,  Gamma = Gamma' + D + Gamma'',  sigma = sigma' + D + sigma''
//     |Gamma'| = |Gamma''|,  |sigma'| = |sigma''|
//    Thus, empirical probability becomes
//     emp_prob = (2|sigma'(n)| + |r|) / (2|Gamma'| + |r|)

/// @brief Functional Probabilistic Approximate Constraints verifier.
/// FDPACVerifier<true> is C++/Python version, which takes @c vectors of @c std::functions as
/// metrics. FDPACVerifier<false> is CLI version, which always uses default metrics.
class FDPACVerifier final : public PairWisePACVerifier {
private:
    using Pairs = std::vector<TuplePair>;
    using Types = std::vector<model::Type const*>;
    using Metrics = std::vector<std::shared_ptr<config::ICustomMetric>>;

    constexpr static double kDefaultLhsDelta = kDistThreshold;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    std::shared_ptr<Types> lhs_types_;
    std::shared_ptr<Types> rhs_types_;

    Metrics lhs_metrics_;
    Metrics rhs_metrics_;

    std::vector<double> lhs_Deltas_;

    std::shared_ptr<pac::model::Tuples> lhs_tuples_;
    std::shared_ptr<pac::model::Tuples> rhs_tuples_;

    // Gamma is a set of pairs such that |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X.
    // This vector holds a subset Gamma' of Gamma (see notes), sorted by max{|t_i[B_l] - t_j[B_l]}
    std::shared_ptr<Pairs> sorted_gamma_;

    std::optional<model::FDPAC> pac_;

    double GetDelta(std::size_t num_pairs) const override;
    double GetNumPairs(double delta) const override;

    /// @brief Fill sorted_gamma. Called in Execute, because Deltas is an execute option.
    void PreparePairs();

    void PreparePACTypeData() override;
    void PACTypeExecuteInternal() override;

    std::pair<double, double> GetEpsilonDeltaForEpsilon(double epsilon) const override {
        return GetEpsilonDeltaForEpsilonImpl(epsilon, *sorted_gamma_);
    }

    void MakeExecuteOptsAvailable() override {
        PACVerifier::MakeExecuteOptsAvailable();
        MakeOptionsAvailable({config::names::kLhsDeltas});
    }

    void ResetState() override {
        pac_ = std::nullopt;
    }

public:
    FDPACVerifier();

    model::FDPAC const& GetPAC() const {
        if (!pac_) {
            throw std::runtime_error("Execute must be called before GetPAC");
        }
        return *pac_;
    }

    FDPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};
}  // namespace algos::pac_verifier
