#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/pac/pac_verifier/pac_verifier.h"
#include "indices/type.h"
#include "pac/model/comparable_tuple_type.h"
#include "pac/model/idomain.h"

namespace algos::pac_verifier {
class DomainPACVerifier final : public PACVerifier {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;

    config::IndicesType column_indices_;

    std::shared_ptr<pac::model::ComparableTupleType> tuple_type_;
    std::shared_ptr<pac::model::IDomain> domain_;

    std::shared_ptr<Tuples> original_value_tuples_;
    std::vector<TuplesIter> sorted_value_tuples_;
    // Iterators in value_tuples_ that fall into Domain itself (i. e. with epsilon = 0)
    std::vector<TuplesIter>::iterator first_value_it_, last_value_it_;

    void RegisterDomainPACOptions();

protected:
    virtual void ProcessPACTypeOptions() override;
    virtual void MakePACTypeExecuteOptionsAvailable() override;
    virtual void PreparePACTypeData() override;
    virtual std::vector<std::size_t> CountSatisfyingTuples(double min_eps, double max_eps,
                                                           unsigned long eps_steps) override;

    virtual void ResetPACTypeState() override {
        tuple_type_ = nullptr;
        original_value_tuples_ = nullptr;
        sorted_value_tuples_ = {};
    }

public:
    DomainPACVerifier() : PACVerifier() {
        using namespace config::names;

        RegisterDomainPACOptions();
    }
};
}  // namespace algos::pac_verifier
