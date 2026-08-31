#include "edit_distance_calculator.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

#include "core/algorithms/sd/util/numeric_utils.h"

namespace algos::sd::util {
namespace {

[[nodiscard]] bool AlmostEqualLongDouble(long double lhs, long double rhs) {
    if (!std::isfinite(lhs) || !std::isfinite(rhs)) {
        return false;
    }
    return std::abs(lhs - rhs) <= ComparisonTolerance(lhs, rhs);
}

[[nodiscard]] bool LessOrAlmostEqual(long double lhs, long double rhs) {
    return lhs <= rhs || AlmostEqualLongDouble(lhs, rhs);
}

[[nodiscard]] long double BoundaryTolerance(long double distance) {
    return ComparisonTolerance(distance, distance);
}

[[nodiscard]] long double CircularDistance(long double lhs, long double rhs, long double period) {
    long double const direct_distance = std::abs(lhs - rhs);
    return std::min(direct_distance, period - direct_distance);
}

}  // namespace

EditDistanceCalculator::EditDistanceCalculator(double g1, double g2) : g1_(g1), g2_(g2) {
    if (!std::isfinite(g1_) || !std::isfinite(g2_)) {
        throw std::invalid_argument("SD gap bounds must be finite.");
    }
    if (g1_ < 0.0) {
        throw std::invalid_argument("SD lower gap bound must be non-negative.");
    }
    if (g2_ >= 0.0 && (g2_ == 0.0 || g2_ < g1_)) {
        throw std::invalid_argument(
                "SD upper gap bound must be positive and no smaller than the lower bound, or "
                "negative for infinity.");
    }
}

long EditDistanceCalculator::CalculateDCostCapped(long double distance,
                                                  long max_relevant_cost) const {
    if (!std::isfinite(distance)) {
        throw std::invalid_argument("SD value difference must be finite.");
    }
    if (max_relevant_cost < 1) {
        return max_relevant_cost + 1;
    }
    if (distance < 0.0L) {
        return -1;
    }
    if (distance == 0.0L && g1_ > 0.0) {
        return -1;
    }

    if (g2_ < 0) {
        return LessOrAlmostEqual(g1_, distance) ? 1 : -1;
    }

    long double const upper_gap = g2_;
    long double const largest_relevant_distance =
            static_cast<long double>(max_relevant_cost) * upper_gap;
    if (!LessOrAlmostEqual(distance, largest_relevant_distance)) {
        return max_relevant_cost + 1;
    }

    long double quotient = distance / upper_gap;
    if (!std::isfinite(quotient)) {
        return max_relevant_cost + 1;
    }

    long double steps = std::max(std::floor(quotient), 1.0L);
    if (!LessOrAlmostEqual(distance, steps * upper_gap)) {
        steps += 1.0L;
    }

    if (steps > static_cast<long double>(max_relevant_cost)) {
        return max_relevant_cost + 1;
    }
    if (!LessOrAlmostEqual(steps * static_cast<long double>(g1_), distance)) {
        return -1;
    }

    return static_cast<long>(steps);
}

long EditDistanceCalculator::CalculateDCost(long double distance) const {
    if (!std::isfinite(distance)) {
        throw std::invalid_argument("SD value difference must be finite.");
    }
    long constexpr largest_representable_cost = std::numeric_limits<long>::max();
    return CalculateDCostCapped(distance, largest_representable_cost - 1);
}

void EditDistanceCalculator::InitGapRemainders(
        std::span<double const> values, double gap, std::vector<int>& class_id,
        std::vector<long double>& remainders, std::vector<long double>& snapped_values,
        std::vector<std::pair<long double, long double>>& class_remainder_bounds,
        std::vector<std::vector<double>>& class_vals, std::vector<Fenwick>& fenwicks) const {
    size_t const n = values.size();
    long double const exact_gap = gap;
    long double const boundary_normalization_tolerance = BoundaryTolerance(exact_gap) / 2.0L;
    std::vector<std::pair<long double, size_t>> sorted_remainders(n);
    remainders.resize(n);
    snapped_values.resize(n);
    for (size_t i = 0; i < n; ++i) {
        long double const value = values[i];
        long double remainder = std::fmod(value, exact_gap);
        long double snapped_value = value - remainder;
        if (remainder < 0) {
            remainder += exact_gap;
            snapped_value -= exact_gap;
        }
        if (exact_gap - remainder <= boundary_normalization_tolerance) {
            remainder = 0.0L;
            snapped_value += exact_gap;
        } else if (remainder <= boundary_normalization_tolerance) {
            remainder = 0.0L;
        }
        remainders[i] = remainder;
        snapped_values[i] = snapped_value;
        sorted_remainders[i] = {remainder, i};
    }
    std::sort(sorted_remainders.begin(), sorted_remainders.end());

    // A fixed representative prevents a chain of near-equal remainders from merging distant ones.
    int cid = 0;
    long double class_representative = sorted_remainders[0].first;
    class_id[sorted_remainders[0].second] = cid;
    for (size_t i = 1; i < n; ++i) {
        if (!AlmostEqualLongDouble(sorted_remainders[i].first, class_representative)) {
            ++cid;
            class_representative = sorted_remainders[i].first;
        }
        class_id[sorted_remainders[i].second] = cid;
    }

    int const classes_count = cid + 1;
    class_vals.resize(classes_count);
    class_remainder_bounds.assign(classes_count, {std::numeric_limits<long double>::infinity(),
                                                  -std::numeric_limits<long double>::infinity()});
    for (size_t i = 0; i < n; ++i) {
        int const c = class_id[i];
        class_vals[c].push_back(values[i]);
        class_remainder_bounds[c].first = std::min(class_remainder_bounds[c].first, remainders[i]);
        class_remainder_bounds[c].second =
                std::max(class_remainder_bounds[c].second, remainders[i]);
    }
    for (int c = 0; c < classes_count; ++c) {
        std::sort(class_vals[c].begin(), class_vals[c].end());
        class_vals[c].erase(std::unique(class_vals[c].begin(), class_vals[c].end()),
                            class_vals[c].end());
        fenwicks.emplace_back(class_vals[c].size());
    }
}

void EditDistanceCalculator::InitIntervalGap(std::span<double const> values,
                                             std::vector<double>& unique_vals,
                                             std::unique_ptr<SegmentTree>& tree,
                                             std::vector<CostNode>& best_prefix_costs) const {
    unique_vals.assign(values.begin(), values.end());
    std::sort(unique_vals.begin(), unique_vals.end());
    unique_vals.erase(std::unique(unique_vals.begin(), unique_vals.end()), unique_vals.end());

    tree = std::make_unique<SegmentTree>(unique_vals.size());
    best_prefix_costs.assign(unique_vals.size(),
                             {std::numeric_limits<long double>::infinity(), -1, 0, 0});
}

void EditDistanceCalculator::UpdateMin2Interval(double current_val, size_t i, size_t l_rank,
                                                size_t r_rank,
                                                std::vector<double> const& unique_vals,
                                                SegmentTree const& tree, long& min2_cost,
                                                long& min2_j) const {
    auto res = tree.Query(l_rank, r_rank);
    if (!res.has_value()) {
        return;
    }

    long const i_as_long = static_cast<long>(i);
    long const max_relevant_dcost = i_as_long + 1;
    long const dcost = CalculateDCostCapped(
            static_cast<long double>(current_val) - unique_vals[res->rank], max_relevant_dcost);
    if (dcost < 1 || dcost > max_relevant_dcost) {
        return;
    }

    long const deletions = i_as_long - 1 - res->j_val;
    long const cost_without_insertions = res->t_val + deletions;
    if (cost_without_insertions > i_as_long - (dcost - 1)) {
        return;
    }
    long const cost = cost_without_insertions + (dcost - 1);
    if (min2_cost == -1 || cost <= min2_cost) {
        min2_cost = cost;
        min2_j = res->source_idx;
    }
}

// Searches predecessor bands for k steps and stops when the bands merge.
void EditDistanceCalculator::ProcessIntervalBands(double current_val, size_t i,
                                                  std::vector<double> const& unique_vals,
                                                  SegmentTree const& tree, long& min2_cost,
                                                  long& min2_j) const {
    if (g2_ < 0) {
        long double const min_distance = g1_;
        long double const upper_value =
                std::min(static_cast<long double>(current_val),
                         static_cast<long double>(current_val) - min_distance +
                                 BoundaryTolerance(min_distance));
        auto it = std::upper_bound(unique_vals.begin(), unique_vals.end(), upper_value);
        if (g1_ > 0.0) {
            it = std::min(it,
                          std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val));
        }
        if (it != unique_vals.begin()) {
            UpdateMin2Interval(current_val, i, 0, std::distance(unique_vals.begin(), it),
                               unique_vals, tree, min2_cost, min2_j);
        }
        return;
    }

    if (g1_ == 0.0) {
        auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val);
        if (it != unique_vals.begin()) {
            UpdateMin2Interval(current_val, i, 0, std::distance(unique_vals.begin(), it),
                               unique_vals, tree, min2_cost, min2_j);
        }
        return;
    }

    long const max_relevant_band = static_cast<long>(i) + 1;
    for (long k = 1; k <= max_relevant_band; ++k) {
        long double const k_as_long_double = k;
        long double const min_distance = k_as_long_double * static_cast<long double>(g1_);
        long double const max_distance = k_as_long_double * static_cast<long double>(g2_);
        if (!std::isfinite(min_distance) || !std::isfinite(max_distance)) {
            break;
        }

        long double const high_val = static_cast<long double>(current_val) - min_distance;
        long double const high_limit = std::min(static_cast<long double>(current_val),
                                                high_val + BoundaryTolerance(min_distance));
        if (high_limit < unique_vals.front()) {
            break;
        }
        long double const low_val = static_cast<long double>(current_val) - max_distance;

        auto it_low = std::lower_bound(unique_vals.begin(), unique_vals.end(),
                                       low_val - BoundaryTolerance(max_distance));
        auto it_high = std::upper_bound(unique_vals.begin(), unique_vals.end(), high_limit);
        it_high = std::min(it_high,
                           std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val));
        if (it_low < it_high) {
            UpdateMin2Interval(current_val, i, std::distance(unique_vals.begin(), it_low),
                               std::distance(unique_vals.begin(), it_high), unique_vals, tree,
                               min2_cost, min2_j);
        }

        long double const covered_overlap =
                k_as_long_double * (static_cast<long double>(g2_) - g1_);
        if (LessOrAlmostEqual(g1_, covered_overlap)) {
            auto it_rest = std::upper_bound(unique_vals.begin(), unique_vals.end(), high_limit);
            it_rest = std::min(
                    it_rest, std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val));
            if (it_rest != unique_vals.begin()) {
                UpdateMin2Interval(current_val, i, 0, std::distance(unique_vals.begin(), it_rest),
                                   unique_vals, tree, min2_cost, min2_j);
            }
            break;
        }
    }
}

template <bool TrackTrace>
EditDistanceCalculator::CalculationResult EditDistanceCalculator::Calculate(
        std::span<double const> values) const {
    CalculationResult result;
    size_t const n = values.size();
    result.is_exact_gap = g1_ > 0.0 && g1_ == g2_;

    if (n >= static_cast<size_t>(std::numeric_limits<long>::max())) {
        throw std::length_error("SD input is too large for edit-distance indices.");
    }
    if (std::any_of(values.begin(), values.end(),
                    [](double value) { return !std::isfinite(value); })) {
        throw std::invalid_argument("SD values must be finite.");
    }

    if (n == 0) {
        return result;
    }
    if (n == 1) {
        result.prefix_ops = {0};
        if constexpr (TrackTrace) {
            result.t_prev.assign(1, -1);
            result.ops_from_t.assign(1, true);
        }
        return result;
    }

    std::vector<int> class_id(n, 0);
    std::vector<long double> remainders;
    std::vector<long double> snapped_values;
    std::vector<std::pair<long double, long double>> class_remainder_bounds;
    std::vector<std::vector<double>> class_vals;
    std::vector<Fenwick> fenwicks;
    std::vector<std::vector<size_t>> seen_class_members;
    std::vector<double> unique_vals;
    std::unique_ptr<SegmentTree> tree;
    std::vector<CostNode> best_prefix_costs;  // Minimum T(j) - j for each compressed value.
    bool const use_gap_remainders = g2_ > 0.0;

    if (!result.is_exact_gap) {
        InitIntervalGap(values, unique_vals, tree, best_prefix_costs);
    }
    if (use_gap_remainders) {
        InitGapRemainders(values, g2_, class_id, remainders, snapped_values, class_remainder_bounds,
                          class_vals, fenwicks);
        seen_class_members.resize(class_vals.size());
    }

    // t[i] keeps element i, while prefix_ops[i] may delete it.
    std::vector<long> t(n, 0);
    result.prefix_ops.assign(n, 0);
    if constexpr (TrackTrace) {
        result.t_prev.assign(n, -1);
        result.ops_from_t.assign(n, true);
    }

    if (use_gap_remainders) {
        int const c0 = class_id[0];
        auto it = std::lower_bound(class_vals[c0].begin(), class_vals[c0].end(), values[0]);
        fenwicks[c0].Update(std::distance(class_vals[c0].begin(), it), -snapped_values[0], 0, 0, 0);
        seen_class_members[c0].push_back(0);
    }
    if (!result.is_exact_gap) {
        auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), values[0]);
        size_t const rank = std::distance(unique_vals.begin(), it);
        best_prefix_costs[rank] = {0, 0, 0, 0};
        long double const key = (g2_ < 0) ? 0.0L : -unique_vals[rank];
        tree->Update(rank, key, rank, 0, 0, 0);
    }

    for (size_t i = 1; i < n; ++i) {
        long const min1_cost = static_cast<long>(i);  // Delete every preceding element.
        long min2_cost = -1;                          // Keep a predecessor and bridge the gap.
        long min2_j = -1;
        long min3_cost = -1;  // Chain an equal value when the minimum gap is zero.
        long min3_j = -1;
        double const current_val = values[i];

        if (!result.is_exact_gap) {
            ProcessIntervalBands(current_val, i, unique_vals, *tree, min2_cost, min2_j);
        }

        if (use_gap_remainders) {
            int const c = class_id[i];
            long const i_as_long = static_cast<long>(i);
            long const max_relevant_dcost = i_as_long + 1;

            auto consider_predecessor = [&](long predecessor_idx, long predecessor_t) {
                long const dcost = CalculateDCostCapped(
                        static_cast<long double>(current_val) - values[predecessor_idx],
                        max_relevant_dcost);
                if (dcost < 1 || dcost > max_relevant_dcost) {
                    return false;
                }

                long const deletions = i_as_long - 1 - predecessor_idx;
                long const cost_without_insertions = predecessor_t + deletions;
                if (cost_without_insertions > i_as_long - (dcost - 1)) {
                    return false;
                }

                long const cost = cost_without_insertions + (dcost - 1);
                if (min2_cost == -1 || cost < min2_cost) {
                    min2_cost = cost;
                    min2_j = predecessor_idx;
                }
                return true;
            };

            enum class ClassQueryResult { kNoCandidate, kAccepted, kRejected };
            auto query_class = [&](int query_class_id) {
                long double const min_distance = g1_;
                long double const max_predecessor =
                        std::min(static_cast<long double>(current_val),
                                 static_cast<long double>(current_val) - min_distance +
                                         BoundaryTolerance(min_distance));
                auto end = std::upper_bound(class_vals[query_class_id].begin(),
                                            class_vals[query_class_id].end(), max_predecessor);
                end = std::min(end,
                               std::lower_bound(class_vals[query_class_id].begin(),
                                                class_vals[query_class_id].end(), current_val));
                size_t const end_rank = std::distance(class_vals[query_class_id].begin(), end);
                if (end_rank == 0) {
                    return ClassQueryResult::kNoCandidate;
                }

                auto min_res = fenwicks[query_class_id].Query(end_rank - 1);
                if (!min_res.has_value()) {
                    return ClassQueryResult::kNoCandidate;
                }
                return consider_predecessor(min_res->j_val, min_res->t_val)
                               ? ClassQueryResult::kAccepted
                               : ClassQueryResult::kRejected;
            };

            auto scan_class = [&](int scan_class_id) {
                for (size_t predecessor : seen_class_members[scan_class_id]) {
                    consider_predecessor(static_cast<long>(predecessor), t[predecessor]);
                }
            };

            // Every pair inside a fixed-representative class is tolerance-compatible. A valid
            // pair may still straddle a class boundary because approximate equality is not
            // transitive, so inspect the neighboring tolerance window as well.
            if (query_class(c) == ClassQueryResult::kRejected) {
                scan_class(c);
            }

            long double const exact_gap = g2_;
            long double const current_remainder = remainders[i];
            long double const max_relevant_distance =
                    static_cast<long double>(max_relevant_dcost) * exact_gap;
            long double const remainder_window =
                    std::min(exact_gap / 2.0L, 2.0L * BoundaryTolerance(max_relevant_distance));
            long double const always_compatible_window = BoundaryTolerance(exact_gap);

            auto inspect_class = [&](int inspect_class_id) {
                if (inspect_class_id == c || seen_class_members[inspect_class_id].empty()) {
                    return;
                }
                auto const [class_min_remainder, class_max_remainder] =
                        class_remainder_bounds[inspect_class_id];
                bool const wholly_compatible =
                        CircularDistance(current_remainder, class_min_remainder, exact_gap) <=
                                always_compatible_window &&
                        CircularDistance(current_remainder, class_max_remainder, exact_gap) <=
                                always_compatible_window;
                if (!wholly_compatible ||
                    query_class(inspect_class_id) == ClassQueryResult::kRejected) {
                    scan_class(inspect_class_id);
                }
            };

            auto inspect_remainder_range = [&](long double low, long double high) {
                auto first = std::lower_bound(class_remainder_bounds.begin(),
                                              class_remainder_bounds.end(), low,
                                              [](auto const& bounds, long double value) {
                                                  return bounds.second < value;
                                              });
                auto last = std::upper_bound(
                        first, class_remainder_bounds.end(), high,
                        [](long double value, auto const& bounds) { return value < bounds.first; });
                for (auto it = first; it != last; ++it) {
                    inspect_class(
                            static_cast<int>(std::distance(class_remainder_bounds.begin(), it)));
                }
            };

            if (remainder_window >= exact_gap / 2.0L) {
                inspect_remainder_range(0.0L, exact_gap);
            } else if (current_remainder < remainder_window) {
                inspect_remainder_range(0.0L, current_remainder + remainder_window);
                inspect_remainder_range(current_remainder - remainder_window + exact_gap,
                                        exact_gap);
            } else if (current_remainder + remainder_window >= exact_gap) {
                inspect_remainder_range(0.0L, current_remainder + remainder_window - exact_gap);
                inspect_remainder_range(current_remainder - remainder_window, exact_gap);
            } else {
                inspect_remainder_range(current_remainder - remainder_window,
                                        current_remainder + remainder_window);
            }
        }

        if (!result.is_exact_gap && g1_ == 0.0) {
            auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val);
            size_t const rank = std::distance(unique_vals.begin(), it);
            if (best_prefix_costs[rank].source_idx != -1) {
                CostNode const& best = best_prefix_costs[rank];
                long const deletions = static_cast<long>(i) - 1 - best.j_val;
                min3_cost = best.t_val + deletions;
                min3_j = best.source_idx;
            }
        }

        t[i] = min1_cost;
        if constexpr (TrackTrace) {
            result.t_prev[i] = -1;
        }
        if (min2_cost != -1 && min2_cost <= t[i]) {
            t[i] = min2_cost;
            if constexpr (TrackTrace) {
                result.t_prev[i] = min2_j;
            }
        }
        if (g1_ == 0.0 && min3_cost != -1 && min3_cost <= t[i]) {
            t[i] = min3_cost;
            if constexpr (TrackTrace) {
                result.t_prev[i] = min3_j;
            }
        }

        if (result.prefix_ops[i - 1] + 1 < t[i]) {
            result.prefix_ops[i] = result.prefix_ops[i - 1] + 1;
            if constexpr (TrackTrace) {
                result.ops_from_t[i] = false;
            }
        } else {
            result.prefix_ops[i] = t[i];
            if constexpr (TrackTrace) {
                result.ops_from_t[i] = true;
            }
        }

        if (use_gap_remainders) {
            int const c = class_id[i];
            auto it = std::lower_bound(class_vals[c].begin(), class_vals[c].end(), current_val);
            long double const key = static_cast<long double>(g2_) * (t[i] - static_cast<long>(i)) -
                                    snapped_values[i];
            fenwicks[c].Update(std::distance(class_vals[c].begin(), it), key, static_cast<long>(i),
                               t[i], static_cast<long>(i));
            seen_class_members[c].push_back(i);
        }
        if (!result.is_exact_gap) {
            auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val);
            size_t const rank = std::distance(unique_vals.begin(), it);
            long const t_minus_i = t[i] - static_cast<long>(i);
            if (static_cast<long double>(t_minus_i) < best_prefix_costs[rank].cost) {
                best_prefix_costs[rank] = {static_cast<long double>(t_minus_i),
                                           static_cast<long>(i), t[i], static_cast<long>(i)};
                long double const key =
                        (g2_ < 0) ? static_cast<long double>(t_minus_i)
                                  : static_cast<long double>(g2_) * t_minus_i - current_val;
                tree->Update(rank, key, rank, static_cast<long>(i), t[i], static_cast<long>(i));
            }
        }
    }

    return result;
}

std::vector<long> EditDistanceCalculator::CalculatePrefixOps(std::span<double const> values) const {
    return Calculate<false>(values).prefix_ops;
}

EditDistanceTrace EditDistanceCalculator::CalculateTrace(std::span<double const> values) const {
    auto result = Calculate<true>(values);
    EditDistanceTrace trace;
    trace.ops = result.prefix_ops.empty() ? 0 : result.prefix_ops.back();
    trace.t_prev = std::move(result.t_prev);
    trace.ops_from_t = std::move(result.ops_from_t);
    trace.is_exact_gap = result.is_exact_gap;
    return trace;
}

}  // namespace algos::sd::util
