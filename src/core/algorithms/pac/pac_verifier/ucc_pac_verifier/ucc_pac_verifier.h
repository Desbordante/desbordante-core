#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/pairwise_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/algorithms/pac/ucc_pac.h"
#include "core/config/custom_metric/custom_vector_metric.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"

namespace algos::pac_verifier {
/// @brief Unique Column Combination Probabilistic Approximate Constraint verifier
/// UCC PAC on column set X specifies that
///   Pr(dist(t_i[X], t_j[X]) <= eps) <= delta
/// (note that this definition differs from original UK PAC definition,
/// which allows only single-column PACs)
// Key ideas:
// 1. As in FD PAC verifier, we consider only tuple pairs (t_i, t_j) such that i < j
//    Consider the following sets:
//      D = {(t_i, t_i) in r^2} -- diagonal
//      Gamma = r^2
//      Gamma' = {(t_i, t_j) in r^2 | i < j},  Gamma'' = {(t_i, t_j) in r^2 | i > j}
//      sigma(eps) = {(t_i, t_j) in r^2 | dist(t_i[X], t_j[X]) <= eps}
//      sigma' = sigma \cap Gamma',  sigma'' = sigma \cap Gamma''
//    Thus, sigma = sigma' + D + sigma''
//    Therefore, empirical probability becomes
//      Pr(eps) = |sigma| / |Gamma| = (2 * |sigma'| + |r|) / |r|^2
class UCCPACVerifier final : public PairWisePACVerifier {
    using Pairs = std::vector<TuplePair>;

    config::IndicesType column_indices_;
    std::shared_ptr<config::ICustomVectorMetric> metric_;
    double max_delta_;

    std::shared_ptr<pac::model::TupleType> tuple_type_;
    std::shared_ptr<std::vector<pac::model::Tuple>> tuples_;

    std::shared_ptr<Pairs> sorted_pairs_;
    std::optional<model::UCCPAC> pac_;

    double GetNumPairs(double delta) const override;
    double GetDelta(std::size_t num_pairs) const override;

    /// @brief Fill sorted_pairs_
    void PreparePairs();

    void ProcessPACTypeOptions() override;
    void PreparePACTypeData() override;
    void PACTypeExecuteInternal() override;

    std::pair<double, double> GetEpsilonDeltaForEpsilon(double epsilon) const override {
        return GetEpsilonDeltaForEpsilonImpl(epsilon, *sorted_pairs_);
    }

    void MakeExecuteOptsAvailable() override {
        PACVerifier::MakeExecuteOptsAvailable();
        MakeOptionsAvailable({config::names::kMaxDelta});
    }

    void ResetState() override {
        pac_ = std::nullopt;
    }

    double MaxDelta() const override {
        return max_delta_;
    }

    void SetMaxDelta(double val) override {
        max_delta_ = val;
    }

    void RefineDelta(PairsIt&) const override;

public:
    UCCPACVerifier();

    model::UCCPAC const& GetPAC() const {
        if (!pac_) {
            throw std::runtime_error("Execute must called before GetPAC");
        }
        return *pac_;
    }

    UCCPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};
}  // namespace algos::pac_verifier
