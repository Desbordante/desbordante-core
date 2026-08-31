#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::sd_miner {

enum class IntervalStrategy : char {
    kExact,
    kApproximate,
};

enum class AssemblyStrategy : char {
    kExact,
    kGreedy,
};

struct SDCandidateInterval {
    // Inclusive, zero-based positions in rows sorted by X, Y, then input order.
    size_t left = 0;
    size_t right = 0;
    double confidence = 0.0;
};

struct SDTableauPattern {
    // Inclusive, zero-based boundaries in the same sorted order as the candidates.
    size_t left_position = 0;
    size_t right_position = 0;
    // X-values at the boundaries. With duplicate X-values, use positions to identify the rows.
    double left_x = 0.0;
    double right_x = 0.0;
    // Number of rows in the interval.
    size_t support = 0;
    // Confidence metric: (N - OPS) / N, where N is the interval's support.
    double confidence = 0.0;
};

class SDMiner final : public Algorithm {
private:
    config::InputTable input_table_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    double g1_ = 0.0;
    // Negative means infinity.
    double g2_ = -1.0;
    // Requested confidence before relaxation by the approximate strategy.
    double min_confidence_ = 1.0;
    // Minimum fraction of eligible rows covered by the tableau.
    double min_support_ = 1.0;
    double approximation_delta_ = 0.05;
    IntervalStrategy interval_strategy_ = IntervalStrategy::kExact;
    AssemblyStrategy assembly_strategy_ = AssemblyStrategy::kExact;

    std::vector<std::vector<std::string>> raw_data_;
    std::vector<double> sorted_x_values_;
    std::vector<double> sorted_y_values_;
    // Maps sorted positions back to row indices in the input table.
    std::vector<size_t> sorted_row_indices_;

    // Maximal candidate intervals.
    std::vector<SDCandidateInterval> candidates_;
    std::vector<SDTableauPattern> tableau_;
    // Number of distinct rows covered by the tableau.
    size_t global_support_ = 0;

    void RegisterOptions();
    void ExtractSortedData();
    std::vector<SDCandidateInterval> GenerateExactIntervals() const;
    std::vector<SDCandidateInterval> GenerateApproximateIntervals() const;
    std::vector<SDCandidateInterval> AssembleExact(
            std::vector<SDCandidateInterval> const& candidates) const;
    std::vector<SDCandidateInterval> AssembleGreedy(
            std::vector<SDCandidateInterval> const& candidates) const;
    void BuildTableau(std::vector<SDCandidateInterval> const& intervals, size_t support);
    [[nodiscard]] size_t GetTargetSupport() const;

    void ResetState() override {
        sorted_x_values_.clear();
        sorted_y_values_.clear();
        sorted_row_indices_.clear();
        candidates_.clear();
        tableau_.clear();
        global_support_ = 0;
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    void ExecuteInternal() override;

public:
    SDMiner();

    [[nodiscard]] std::vector<SDCandidateInterval> const& GetCandidates() const noexcept {
        return candidates_;
    }

    [[nodiscard]] std::vector<SDTableauPattern> const& GetTableau() const noexcept {
        return tableau_;
    }

    [[nodiscard]] size_t GetGlobalSupport() const noexcept {
        return global_support_;
    }

    [[nodiscard]] std::vector<size_t> const& GetSortedRowIndices() const noexcept {
        return sorted_row_indices_;
    }
};

}  // namespace algos::sd_miner
