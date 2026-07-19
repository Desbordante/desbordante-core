#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::pac_verifier {
/// @brief Base class for Probabilistic Approximate Constraints verifiers
class PACVerifier : public Algorithm {
protected:
    struct EpsilonDelta {
        double epsilon;
        double delta;
    };

private:
    constexpr static double kDefaultMinDelta = 0.9;
    constexpr static double kDefaultMaxDelta = 0.2;
    // Diagonal threshold is the maximum slope coefficient of a segment on the ECDF, that is still
    // considered horizontal during verifying PAC via elbow method
    // See https://colab.research.google.com/drive/1t2i-BgzRaL3VSzL0Q0izR1RbgE1i0Ohu?usp=sharing
    // 1e-5 is one extra value on a table containing 10^5 rows
    constexpr static double kDefaultDiagonalThreshold = 1e-5;

    double min_epsilon_;
    double max_epsilon_;
    double min_delta_ = 0;
    double max_delta_ = 1;
    double diagonal_threshold_;
    unsigned long delta_steps_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    /// @brief Remove extra pairs from empirical_probabilities
    std::ranges::subrange<std::vector<EpsilonDelta>::const_iterator> BuildECDF(
            std::vector<EpsilonDelta>& empirical_probabilities) const;

    /// @brief Check if user requested "validation" (finding one parameter by given value of another
    /// one) and "validate" if needed
    std::optional<EpsilonDelta> TryValidatePAC(
            std::vector<EpsilonDelta> const& empirical_probabilities) const;

    /// @brief Check if there are pairs between min epsilon and max epsilon, and select proper (eps,
    /// delta) if needed
    std::optional<EpsilonDelta> CheckPairsBetweenMinMaxEpsilon(
            std::vector<EpsilonDelta> const& empirical_probabilities) const;

    // Trivial getters intended to be used with STL algorithms
    static double GetEpsilon(EpsilonDelta const& eps_delta) {
        return eps_delta.epsilon;
    }

    static double GetDelta(EpsilonDelta const& eps_delta) {
        return eps_delta.delta;
    }

protected:
    // Input table must be registered in concrete classes to allow setting conditional options on it
    // Therefore, we need a protected data member here
    config::InputTable input_table_;

    // Threshold for floating-point comparison of distances
    constexpr static double kDistThreshold = 1e-12;

    double MinDelta() const {
        return min_delta_;
    }

    double MaxDelta() const {
        return max_delta_;
    }

    std::size_t DeltaSteps() const {
        return delta_steps_;
    }

    model::ColumnLayoutTypedRelationData const& TypedRelation() const {
        return *typed_relation_;
    }

    virtual void LoadDataInternal() override;
    void RegisterCommonOptions(bool has_min_delta_option, bool has_max_delta_option);
    virtual void MakeExecuteOptsAvailable() override;

    /// @brief Prepare data for validating concrete PAC type.
    /// Called after processing options.
    virtual void PreparePACTypeData() {}

    void LogCommonOptions() const;

    /// @brief Find PAC's epsilon and delta using elbow method on ECDF defined by @c
    /// empirical_probabilities (epsilon-delta pairs)
    /// @return epsilon, delta
    EpsilonDelta FindEpsilonDelta(std::vector<EpsilonDelta>&& empirical_probabilities) const;

    /// @brief Get (refined) epsilon-delta pair with specific epsilon
    /// Must return the minimal possible epsilon when the requested @c epsilon is too small to get
    /// epsilon-delta for it
    virtual EpsilonDelta GetEpsilonDeltaForEpsilon(double epsilon) const = 0;
};
}  // namespace algos::pac_verifier
