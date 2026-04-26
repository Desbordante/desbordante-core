#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/algorithms/pac/ucc_pac.h"
#include "core/config/custom_metric/custom_vector_metric.h"
#include "core/config/indices/type.h"

namespace algos::pac_verifier {
/// @brief Unique Column Combination Probabilistic Approximate Constraint verifier
/// UCC PAC on column set X specifies that
///   Pr(dist(t_i[X], t_j[X]) <= eps) <= delta
/// (note that this definition differs from original UK PAC definition,
/// which allows only single-column PACs)
// Key ideas:
// 1. Actually, we verify the "inverse PAC": Pr(dist(t_i[X], t_j[X]) > eps) >= delta_inv,
//    and then take delta = 1 - delta_inv, so that PAC-Man can be applied without modifications
// 2. As in FD PAC verifier, we consider only tuple pairs (t_i, t_j) such that i < j
//    Consider the following sets:
//      D = {(t_i, t_i) in r^2} -- diagonal
//      Gamma = r^2
//      Gamma' = {(t_i, t_j) in r^2 | i < j},  Gamma'' = {(t_i, t_j) in r^2 | i > j}
//      sigma(eps) = {(t_i, t_j) in r^2 | dist(t_i[X], t_j[X]) <= eps}
//      sigma' = sigma \cap Gamma',  sigma'' = sigma \cap Gamma''
//    Thus, Gamma = Gamma' + D + Gamma'',  sigma = sigma' + D + sigma''
//    Therefore, empirical probability becomes
//      Pr(eps) = (2|sigma'(eps)| + |r|) / |r|^2
class UCCPACVerifier final : public PACVerifier {
    using Pairs = std::vector<TuplePair>;

    config::IndicesType column_indices_;
    std::shared_ptr<config::ICustomVectorMetric> metric_;

    std::shared_ptr<pac::model::TupleType> tuple_type_;
    std::shared_ptr<std::vector<pac::model::Tuple>> tuples_;

    std::shared_ptr<Pairs> sorted_pairs_;
    std::optional<model::UCCPAC> pac_;

    double GetNumPairs(double delta) const;
    double GetDelta(std::size_t num_pairs) const;

    /// @brief Fill sorted_pairs_
    void PreparePairs();

    /// @brief For each delta_i find the least eps_i such that PAC_{eps_i}^{delta_i} holds.
    /// Then refine delta_i, i. e. find the greatest delta_i' such that PAC_{eps_i}^{delta_i} holds.
    /// @return (eps_i, delta_i') pairs
    std::vector<std::pair<double, double>> CalculateEmpiricalProbabilities() const;

    void ProcessPACTypeOptions() override;
    void PreparePACTypeData() override;
    void PACTypeExecuteInternal() override;
    std::pair<double, double> GetEpsilonDeltaForEpsilon(double epsilon) const override;

    void MakeExecuteOptsAvailable() override {
        PACVerifier::MakeExecuteOptsAvailable();
        MakeOptionsAvailable({config::names::kLhsDeltas});
    }

    void ResetState() override {
        pac_ = std::nullopt;
    }

public:
    UCCPACVerifier();

    model::UCCPAC const& GetPAC() const {
        if (!pac_) {
            throw std::runtime_error("Execute must called before GetPAC");
        }
        return *pac_;
    }

    // TODO: highlights
    // FDPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};
}  // namespace algos::pac_verifier
