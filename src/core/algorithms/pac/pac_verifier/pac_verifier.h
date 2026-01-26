#pragma once

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/pac/pac.h"
#include "core/config/names.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::pac_verifier {
/// @brief Base class for Probabilistic Approximate Constrains verifiers
class PACVerifier : public Algorithm {
private:
    constexpr static double kDefaultMinDelta = 0.9;
    // One extra value on table containing 10^5 rows
    constexpr static double kDefaultDiagonalThreshold = 1e-5;

    config::InputTable input_table_;
    bool distance_from_null_is_infinity_;
    double min_epsilon_;
    double max_epsilon_;
    double min_delta_;
    double diagonal_threshold_;
    unsigned long delta_steps_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::shared_ptr<model::PAC> pac_;

    void RegisterOptions();

    /// @brief Process execute options that are common for all PAC types.
    void ProcessCommonExecuteOpts();

    unsigned long long ExecuteInternal() final;

protected:
    // Threshold for floating-point comparison of distances
    constexpr static double kDistThreshold = 1e-12;

    bool DistFromNullIsInfty() const {
        return distance_from_null_is_infinity_;
    }

    double MinDelta() const {
        return min_delta_;
    }

    std::size_t DeltaSteps() const {
        return delta_steps_;
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
    virtual void ProcessPACTypeOptions() {}

    /// @brief Prepare data for validating concrete PAC type.
    /// Called after processing options.
    virtual void PreparePACTypeData() {}

    /// @brief ExecuteInternal for concrete PAC types.
    virtual void PACTypeExecuteInternal() = 0;

    /// @brief Find PAC's epsilon and delta using elbow method on ECDF defined by @c
    /// empirical_probabilities (epsilon-delta pairs)
    /// @return epsilon, delta
    std::pair<double, double> FindEpsilonDelta(
            std::vector<std::pair<double, double>>&& empirical_probabilities) const;

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
        if (!pac_) {
            throw std::runtime_error("Cannot get PAC: it's nullptr");
        }
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
