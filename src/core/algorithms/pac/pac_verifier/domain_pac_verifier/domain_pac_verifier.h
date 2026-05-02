#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/config/descriptions.h"
#include "core/config/indices/option.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos::pac_verifier {
/// @brief Domain Probabilistic Approximate Constraint verifier.
class DomainPACVerifier final : public PACVerifier {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;
    using TuplesIteratorsIter = std::vector<TuplesIter>::const_iterator;

    config::IndicesType column_indices_;
    double min_delta_;

    std::shared_ptr<Tuples> original_value_tuples_;
    // Distances from domain to each value, sorted by distance
    std::vector<std::pair<TuplesIter, double>> dists_from_domain_;
    std::shared_ptr<pac::model::TupleType> tuple_type_;
    // Iterator to after-last value that falls into domain itself (with eps = 0)
    std::vector<std::pair<TuplesIter, double>>::const_iterator domain_end_;
    std::shared_ptr<pac::model::IDomain> domain_;

    std::optional<model::DomainPAC> pac_;

    /// @brief For each of @c delta_steps delta_i values in [@c min_delta, 1] find eps_i such that
    /// Pr(eps < eps_i) >= delta_i
    /// Also refine deltas, i. e. find max acceptable delta_i
    /// @return (eps_i, refined delta_i) pairs
    std::vector<std::pair<double, double>> FindEpsilons() const;

protected:
    double MinDelta() const override {
        return min_delta_;
    }

    void SetMinDelta(double val) override {
        min_delta_ = val;
    }

    virtual void ProcessPACTypeOptions() override;
    virtual void PreparePACTypeData() override;
    void PACTypeExecuteInternal() override;
    std::pair<double, double> GetEpsilonDeltaForEpsilon(double epsilon) const override;

    void ResetState() override {
        pac_ = std::nullopt;
    }

    void MakeExecuteOptsAvailable() override {
        PACVerifier::MakeExecuteOptsAvailable();
        MakeOptionsAvailable({config::names::kMinDelta});
    }

public:
    DomainPACVerifier() : PACVerifier() {
        DESBORDANTE_OPTION_USING;

        RegisterOption(
                config::kTableOpt(&input_table_).SetConditionalOpts({{nullptr, {kColumnIndices}}}));

        RegisterOption(config::IndicesOption{kColumnIndices, kDColumnIndices, nullptr}(
                &column_indices_, [this]() { return input_table_->GetNumberOfColumns(); }));
        RegisterOption(Option(&domain_, kDomain, kDDomain));

        RegisterOption(Option(&min_delta_, kMinDelta, kDMinDelta, -1.0).SetValueCheck([](double x) {
            return x <= 1;
        }));

        MakeOptionsAvailable({config::kTableOpt.GetName(), kDomain});
    }

    model::DomainPAC const& GetPAC() const {
        if (!pac_) {
            throw std::runtime_error("Execute must be called before GetPAC");
        }
        return *pac_;
    }

    DomainPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};
}  // namespace algos::pac_verifier
