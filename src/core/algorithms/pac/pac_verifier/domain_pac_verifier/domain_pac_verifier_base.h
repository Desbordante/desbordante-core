#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/idomain.h"
#include "algorithms/pac/model/tuple.h"
#include "algorithms/pac/pac_verifier/pac_highlight.h"
#include "algorithms/pac/pac_verifier/pac_verifier.h"
#include "descriptions.h"
#include "indices/type.h"
#include "names.h"
#include "option_using.h"

namespace algos::pac_verifier {
/// @brief Base class for Domain Probabilistic Approximate Constraints verifier.
/// Domain option is handled in concrete verifiers.
class DomainPACVerifierBase : public PACVerifier {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;

    config::IndicesType column_indices_;

    std::shared_ptr<Tuples> original_value_tuples_;
    std::vector<TuplesIter> sorted_value_tuples_;
    std::shared_ptr<pac::model::ComparableTupleType> tuple_type_;
    // Iterators in value_tuples_ that fall into Domain itself (i. e. with epsilon = 0)
    std::vector<TuplesIter>::iterator first_value_it_, after_last_value_it_;

protected:
    std::shared_ptr<pac::model::IDomain> domain_;

    virtual void ProcessPACTypeOptions() override;
    virtual void PreparePACTypeData() override;
    virtual std::vector<std::size_t> CountSatisfyingTuples(double min_eps, double max_eps,
                                                           unsigned long eps_steps) override;

    virtual std::unique_ptr<PACHighlight> GetHighlightsInternal(double eps_1 = -1,
                                                                double eps_2 = -1) const override;

public:
    DomainPACVerifierBase() : PACVerifier() {
        DESBORDANTE_OPTION_USING;

        RegisterOption(Option(&column_indices_, kColumnIndices, kDColumnIndices));
        MakeOptionsAvailable({kColumnIndices});
    }
};
}  // namespace algos::pac_verifier
