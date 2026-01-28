#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/config/descriptions.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/config/option_using.h"

namespace algos::pac_verifier {
/// @brief Base class for Domain Probabilistic Approximate Constraints verifier.
/// Domain option is handled in concrete verifiers.
class DomainPACVerifierBase : public PACVerifier {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;
    using TuplesIteratorsIter = std::vector<TuplesIter>::const_iterator;

    config::IndicesType column_indices_;

    std::shared_ptr<Tuples> original_value_tuples_;
    // Distances from domain to each value, sorted by distance
    std::vector<std::pair<TuplesIter, double>> dists_from_domain_;
    std::shared_ptr<pac::model::TupleType> tuple_type_;
    // Iterator to after-last value that falls into domain itself (with eps = 0)
    std::vector<std::pair<TuplesIter, double>>::const_iterator domain_end_;

    /// @brief For each of @c delta_steps delta_i values in [@c min_delta, 1] find eps_i such that
    /// Pr(eps < eps_i) >= delta_i
    /// Also refine deltas, i. e. find max acceptable delta_i
    /// @return (eps_i, refined delta_i) pairs
    std::vector<std::pair<double, double>> FindEpsilons() const;

protected:
    std::shared_ptr<pac::model::IDomain> domain_;

    virtual void ProcessPACTypeOptions() override;
    virtual void PreparePACTypeData() override;
    void PACTypeExecuteInternal() override;

public:
    DomainPACVerifierBase() : PACVerifier() {
        DESBORDANTE_OPTION_USING;

        // config::IndicesOption cannot be used here, because it would require indices to be execute
        // option, but this will make data preparation impossible
        RegisterOption(Option(&column_indices_, kColumnIndices, kDColumnIndices));
        MakeOptionsAvailable({kColumnIndices});
    }

    DomainPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};
}  // namespace algos::pac_verifier
