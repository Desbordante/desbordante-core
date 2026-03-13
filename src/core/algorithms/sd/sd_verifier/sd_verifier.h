#pragma once

#include <limits>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/sd/sd_verifier/util/data_structures.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::sd_verifier {

// Exception that represents an element that must be deleted
// in order for the Sequential Dependency (SD) to hold.
struct SDDeletion {
    // Index of the row that needs to be deleted.
    size_t row_idx;
};

// Exception that represents a gap between two valid elements
// that needs to be filled with insertions.
struct SDInsertion {
    // Index of the row on the left side of the gap.
    size_t left_row_idx;
    // Index of the row on the right side of the gap.
    size_t right_row_idx;
    // Value on the left side of the gap.
    double val_left;
    // Value on the right side of the gap.
    double val_right;
    // Minimal number of insertions needed to bridge the gap.
    long min_insertions;
    // Maximal number of insertions needed to bridge the gap.
    long max_insertions;
};

using SDViolation = std::variant<SDDeletion, SDInsertion>;

class SDVerifier final : public Algorithm {
public:
    using Error = double;

private:
    // Input table provided by the user.
    config::InputTable input_table_;
    // Indices of columns to use as X of the SD.
    config::IndicesType lhs_indices_;
    // Indices of columns to use as Y of the SD.
    config::IndicesType rhs_indices_;
    // Subset of row indices to validate (empty means use all rows).
    config::IndicesType indices_;
    // Minimum gap value (G1). G1 >= 0.
    double g1_ = 0.0;
    // Maximum gap value (G2). G2 >= G1 or G2 < 0 for infinity.
    double g2_ = -1.0;
    // X-values extracted from the input table.
    std::vector<double> x_val_;
    // Y-values extracted from the input table.
    std::vector<double> y_val_;
    // Raw string data loaded from the input table before parsing.
    std::vector<std::vector<std::string>> raw_data_;
    // Confidence metric: (N - OPS) / N, where N is the number of rows.
    double confidence_ = 0.0;
    // Minimum number of operations needed.
    long ops_ = 0;
    // List of violations found in the data.
    std::vector<SDViolation> violations_;

    void RegisterOptions();
    long CalculateDCost(double distance) const;
    long CalculateOps(std::vector<double> const& values,
                      std::vector<size_t> const& original_indices);
    void InitExactGap(size_t n, std::vector<double> const& values, std::vector<int>& class_id,
                      std::vector<std::vector<double>>& class_vals,
                      std::vector<ds::Fenwick>& fenwicks) const;
    void InitIntervalGap(std::vector<double> const& values, std::vector<double>& unique_vals,
                         std::unique_ptr<ds::SegmentTree>& tree,
                         std::vector<ds::CostNode>& best_prefix_costs) const;
    void UpdateMin2Interval(double current_val, size_t i, size_t l_rank, size_t r_rank,
                            std::vector<double> const& unique_vals, ds::SegmentTree const& tree,
                            long& min2_cost, long& min2_j) const;
    void ProcessIntervalBands(double current_val, size_t i, std::vector<double> const& unique_vals,
                              ds::SegmentTree const& tree, long& min2_cost, long& min2_j) const;
    void ReconstructPath(std::vector<double> const& values,
                         std::vector<size_t> const& original_indices,
                         std::vector<bool> const& ops_from_t, std::vector<long> const& t_prev,
                         bool is_exact_gap);

    void ResetState() override {
        confidence_ = 0.0;
        ops_ = 0;
        violations_.clear();
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    SDVerifier();

    [[nodiscard]] long GetOPS() const noexcept {
        return ops_;
    }

    [[nodiscard]] std::vector<SDViolation> const& GetViolations() const noexcept {
        return violations_;
    }

    [[nodiscard]] Error GetError() const noexcept {
        return 1.0 - confidence_;
    }

    [[nodiscard]] double GetConfidence() const noexcept {
        return confidence_;
    }

    [[nodiscard]] bool Holds(Error error) const noexcept {
        return ((1.0 - confidence_) <= error);
    }
};

}  // namespace algos::sd_verifier
