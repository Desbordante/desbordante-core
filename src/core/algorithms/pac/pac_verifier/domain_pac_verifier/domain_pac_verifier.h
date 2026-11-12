#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/pac_man.h"
#include "core/algorithms/pac/pac_verifier/util/pac_verifier_utils.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::pac_verifier {
/// @brief Domain Probabilistic Approximate Constraint verifier.
class DomainPACVerifier final : public Algorithm {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::const_iterator;
    using TuplesIteratorsIter = std::vector<TuplesIter>::const_iterator;

    config::IndicesType column_indices_;
    std::shared_ptr<pac::model::IDomain> domain_;

    config::InputTable input_table_;
    double min_epsilon_;
    double max_epsilon_;
    double min_delta_;
    double diagonal_threshold_;
    unsigned long delta_steps_;

    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    std::shared_ptr<Tuples> original_value_tuples_;
    // Distances from domain to each value, sorted by distance
    std::vector<std::pair<TuplesIter, double>> dists_from_domain_;
    std::shared_ptr<pac::model::TupleType> tuple_type_;
    // Iterator to after-last value that falls into domain itself (with eps = 0)
    std::vector<std::pair<TuplesIter, double>>::const_iterator domain_end_;

    std::unique_ptr<PACMan> pac_man_;

    std::optional<model::DomainPAC> pac_;

    /// @brief For each of @c delta_steps delta_i values in [@c min_delta, 1] find eps_i such that
    /// Pr(eps < eps_i) >= delta_i
    /// Also refine deltas, i. e. find max acceptable delta_i
    /// @return (eps_i, refined delta_i) pairs
    std::vector<EpsilonDelta> FindEpsilons() const;

    void MakeDomain();
    void ExecuteInternal() override;
    EpsilonDelta GetEpsilonDeltaForEpsilon(double epsilon) const;

    void ResetState() override {
        pac_ = std::nullopt;
        pac_man_ = nullptr;
    }

    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;

public:
    DomainPACVerifier();

    model::DomainPAC const& GetPAC() const {
        if (!pac_) {
            throw std::runtime_error("Execute must be called before GetPAC");
        }
        return *pac_;
    }

    DomainPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};
}  // namespace algos::pac_verifier
