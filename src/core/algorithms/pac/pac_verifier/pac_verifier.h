#pragma once

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "algorithm.h"
#include "algorithms/pac/pac.h"
#include "names.h"
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

    /// @brief Process options for concrete PAC type
    /// @note As a side effect, this metrhod should initialize @c pac_ and may initialize
    /// private fields of derived classes
    virtual void ProcessPACTypeOptions() {}

    /// @brief Prepare data for validating concrete PAC type.
    /// Called after processing options.
    virtual void PreparePACTypeData() {}

    /// @brief Find PAC's epsilon and delta using elbow method on ECDF defined by @c
    /// empirical_probabilities
    /// @return epsilon, delta
    std::pair<double, double> FindEpsilonDelta(
            std::vector<double> const& empirical_probabilities) const;

    void ResetState() override {
        pac_ = nullptr;
    }

public:
    PACVerifier() : Algorithm({}) {
        RegisterOptions();
        MakeOptionsAvailable({config::kTableOpt.GetName(), config::names::kDistFromNullIsInfinity});
    }

    virtual ~PACVerifier() = default;

    model::PAC const& GetPAC() const {
        return *pac_;
    }

    model::PAC& GetPAC() {
        if (!pac_) {
            throw std::runtime_error("Cannot get PAC: it's nullptr");
        }
        return *pac_;
    }

    std::shared_ptr<model::PAC> GetPACPtr() {
        return pac_;
    }
};
}  // namespace algos::pac_verifier
