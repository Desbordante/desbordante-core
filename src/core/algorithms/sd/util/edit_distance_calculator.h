#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "core/algorithms/sd/util/data_structures.h"

namespace algos::sd::util {

struct EditDistanceTrace {
    // Minimum number of insertions and deletions for the whole sequence.
    long ops = 0;
    // Predecessor in the optimal path ending at each element, or -1 if there is none.
    std::vector<long> t_prev;
    // True if the optimal prefix keeps its last element instead of deleting it.
    std::vector<bool> ops_from_t;
    bool is_exact_gap = false;
};

class EditDistanceCalculator {
private:
    struct CalculationResult {
        std::vector<long> prefix_ops;
        std::vector<long> t_prev;
        std::vector<bool> ops_from_t;
        bool is_exact_gap = false;
    };

    double g1_;
    double g2_;

    // Returns max_relevant_cost + 1 when a bridge would be too costly to improve the DP.
    [[nodiscard]] long CalculateDCostCapped(long double distance, long max_relevant_cost) const;

    void InitGapRemainders(std::span<double const> values, double gap, std::vector<int>& class_id,
                           std::vector<long double>& remainders,
                           std::vector<long double>& snapped_values,
                           std::vector<std::pair<long double, long double>>& class_remainder_bounds,
                           std::vector<std::vector<double>>& class_vals,
                           std::vector<Fenwick>& fenwicks) const;
    void InitIntervalGap(std::span<double const> values, std::vector<double>& unique_vals,
                         std::unique_ptr<SegmentTree>& tree,
                         std::vector<CostNode>& best_prefix_costs) const;
    void UpdateMin2Interval(double current_val, size_t i, size_t l_rank, size_t r_rank,
                            std::vector<double> const& unique_vals, SegmentTree const& tree,
                            long& min2_cost, long& min2_j) const;
    void ProcessIntervalBands(double current_val, size_t i, std::vector<double> const& unique_vals,
                              SegmentTree const& tree, long& min2_cost, long& min2_j) const;

    template <bool TrackTrace>
    [[nodiscard]] CalculationResult Calculate(std::span<double const> values) const;

public:
    EditDistanceCalculator(double g1, double g2);

    [[nodiscard]] long CalculateDCost(long double distance) const;
    // Minimum operations for each nonempty prefix, without reconstructing a path.
    [[nodiscard]] std::vector<long> CalculatePrefixOps(std::span<double const> values) const;
    // Minimum operations and the path information needed to reconstruct violations.
    [[nodiscard]] EditDistanceTrace CalculateTrace(std::span<double const> values) const;
};

}  // namespace algos::sd::util
