#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::pac_verifier {
/// @brief Base class for Probabilistic Approximate Constrains verifiers
class PACVerifier : public Algorithm {
private:
    constexpr static double kDefaultMinDelta = 0.9;
    // Diagonal threshold is the maximum slope coefficient of a segment on the ECDF, that is still
    // considered horizontal during verifying PAC via elbow method
    // See https://colab.research.google.com/drive/1t2i-BgzRaL3VSzL0Q0izR1RbgE1i0Ohu?usp=sharing
    // 1e-5 is one extra value on a table containing 10^5 rows
    constexpr static double kDefaultDiagonalThreshold = 1e-5;

    double min_epsilon_;
    double max_epsilon_;
    double min_delta_;
    double diagonal_threshold_;
    unsigned long delta_steps_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    void RegisterOptions();

    /// @brief Process execute options that are common for all PAC types.
    void ProcessCommonExecuteOpts();

    unsigned long long ExecuteInternal() final;

protected:
    // Input table must be registered in concrete classes to allow setting conditional options on it
    // Therefore, we need a protected data member here
    config::InputTable input_table_;

    // Threshold for floating-point comparison of distances
    constexpr static double kDistThreshold = 1e-12;

    double MinDelta() const {
        return min_delta_;
    }

    std::size_t DeltaSteps() const {
        return delta_steps_;
    }

    model::ColumnLayoutTypedRelationData const& TypedRelation() const {
        return *typed_relation_;
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

    /// @brief Get (refined) epsilon-delta pair with specific epsilon
    virtual std::pair<double, double> GetEpsilonDeltaForEpsilon(double epsilon) const = 0;

public:
    PACVerifier() : Algorithm() {
        RegisterOptions();
    }

    virtual ~PACVerifier() = default;
};
}  // namespace algos::pac_verifier
