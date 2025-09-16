#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "algorithms/pac/model/metrizable_tuple.h"
#include "algorithms/pac/pac_verifier/pac_verifier.h"
#include "indices/type.h"

namespace algos::pac_verifier {
class DomainPACVerifier final : public PACVerifier {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;

    config::IndicesType column_indices_;

    pac::model::Metric custom_metric_;
    pac::model::Comparer custom_comparer_;
    std::vector<std::string> first_str_, last_str_;

    std::shared_ptr<Tuples> original_value_tuples_;
    std::vector<TuplesIter> sorted_value_tuples_;
    std::shared_ptr<pac::model::MetrizableTupleType> tuple_type_;
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
        original_value_tuples_ = nullptr;
        sorted_value_tuples_ = {};
        tuple_type_ = nullptr;
    }

public:
    DomainPACVerifier() : PACVerifier() {
        using namespace config::names;

        RegisterDomainPACOptions();
    }
};
}  // namespace algos::pac_verifier
