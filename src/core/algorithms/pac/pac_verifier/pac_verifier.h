#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "algorithm.h"
#include "algorithms/pac/pac.h"
#include "names.h"
#include "pac/pac_verifier/pac_highlight.h"
#include "table/column_layout_typed_relation_data.h"
#include "tabular_data/input_table/option.h"
#include "tabular_data/input_table_type.h"

namespace algos::pac_verifier {
/// @brief Base class for Probabilistic Approximate Constrains verifiers
class PACVerifier : public Algorithm {
private:
    constexpr static double kDefaultMinEpsilon = 0;
    constexpr static double kDefaultMaxEpsilon = 1;
    constexpr static unsigned long kDefaultEpsilonSteps = 100;
    constexpr static double kDefaultMinDelta = 0.9;
    // One extra value on table containing 10^5 rows
    constexpr static double kDefaultDiagonalThreshold = 1e-5;

    config::InputTable input_table_;
    bool distance_from_null_is_infinity_;
    double min_epsilon_;
    double max_epsilon_;
    unsigned long epsilon_steps_;
    double min_delta_;
    double diagonal_threshold_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::shared_ptr<model::PAC> pac_;

    void RegisterOptions();

    virtual void ResetState() override {
        pac_->SetEpsilon(0);
        pac_->SetDelta(0);

        ResetPACTypeState();
    }

protected:
    bool DistFromNullIsInfty() const {
        return distance_from_null_is_infinity_;
    }

    double MinEpsilon() const {
        return min_epsilon_;
    }

    double MaxEpsilon() const {
        return max_epsilon_;
    }

    unsigned long EpsilonSteps() const {
        return epsilon_steps_;
    }

    model::ColumnLayoutTypedRelationData const& TypedRelation() const {
        return *typed_relation_;
    }

    template <typename PACT, typename... Args>
    void MakePAC(Args&&... args) {
        pac_ = std::make_shared<PACT>(std::forward<Args>(args)...);
    }

    virtual void LoadDataInternal() override;
    virtual void MakeExecuteOptsAvailable() override;
    virtual unsigned long long ExecuteInternal() override;

    /// @brief Process options for concrete PAC type
    /// @note As a side effect, this metrhod should initialize @c pac_ and may initialize
    /// private fields of derived classes
    virtual void ProcessPACTypeOptions() {}

    /// @brief Prepare data for validating concrete PAC type.
    /// Called after processing options.
    virtual void PreparePACTypeData() {}

    /// @brief For @c eps_steps epsilons in range [@c min_eps, @c max_eps] calculate number of
    /// values that satisfy approximate dependency with this epsilon.
    /// @c i-th place contains number of values for
    /// epsilon = @c min_eps + (@c max_eps - @c min_eps) / @c eps_steps * @c i
    virtual std::vector<std::size_t> CountSatisfyingTuples(double min_eps, double max_eps,
                                                           unsigned long eps_steps) = 0;

    /// @brief Reset state specific for concrete PAC type
    virtual void ResetPACTypeState() {}

    virtual std::unique_ptr<PACHighlight> GetHighlightsInternal(double eps_1,
                                                                double eps_2) const = 0;

public:
    PACVerifier() : Algorithm({}) {
        RegisterOptions();
        MakeOptionsAvailable({config::kTableOpt.GetName(), config::names::kDistFromNullIsInfinity});
    }

    virtual ~PACVerifier() = default;

    model::PAC const& GetPAC() const {
        return *pac_;
    }

    std::shared_ptr<model::PAC> GetPACPtr() {
        return pac_;
    }

    std::unique_ptr<PACHighlight> GetHighlights(double eps_1, double eps_2) const {
        return GetHighlightsInternal(eps_1, eps_2);
    }
};
}  // namespace algos::pac_verifier
