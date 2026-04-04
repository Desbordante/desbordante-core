#include "sd_verifier.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>

#include "core/config/column_index/option.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "util/data_structures.h"

namespace algos::sd_verifier {

SDVerifier::SDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void SDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName(),
                          kSdIndices, kSdG1, kSdG2});
}

void SDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto check_positive = [](double val) {
        if (val < 0) {
            throw std::runtime_error("g1 must be non-negative.");
        }
    };
    auto check_g2 = [this](double val) {
        if (val >= 0 && val < g1_) {
            throw std::runtime_error("g2 must be >= g1 or negative.");
        }
    };

    RegisterOption(config::kTableOpt(&input_table_));

    auto get_cols_num = [this]() {
        return input_table_ ? static_cast<config::IndexType>(input_table_->GetNumberOfColumns())
                            : 0;
    };

    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_cols_num));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_cols_num));
    RegisterOption(
            Option<config::IndicesType>{&indices_, kSdIndices, kDSdIndices}.SetIsRequiredFunc(
                    []() { return false; }));
    RegisterOption(Option<double>{&g1_, kSdG1, kDSdG1}.SetValueCheck(check_positive));
    RegisterOption(Option<double>{&g2_, kSdG2, kDSdG2}.SetValueCheck(check_g2));
}

void SDVerifier::LoadDataInternal() {
    raw_data_.clear();
    if (!input_table_) {
        throw std::runtime_error("Input table is not initialized.");
    }

    input_table_->Reset();
    while (input_table_->HasNextRow()) {
        raw_data_.push_back(input_table_->GetNextRow());
    }
}

// Returns minimum insertions needed to cover distance d within [G1, G2].
// Returns -1 if distance cannot be covered.
long SDVerifier::CalculateDCost(double d) const {
    if (d < 0) {
        return -1;
    }

    // G2 is infinity - one insertion covers any valid distance >= G1
    if (g2_ < 0) {
        return (d >= g1_) ? 1 : -1;
    }

    // General interval [G1, G2] - find minimum k where k*G1 <= d <= k*G2
    long k = static_cast<long>(std::ceil(d / g2_));
    k = std::max(k, 1L);

    if (k * g1_ <= d) {
        return k;
    } else {
        return -1;
    }
}

// Preprocessing for exact gap case (G1 == G2).
// Groups elements by their remainder modulo G1 into equivalence classes.
// Each class gets its own Fenwick tree for O(log N) queries.
void SDVerifier::InitExactGap(size_t n, std::vector<double> const& values,
                              std::vector<int>& class_id,
                              std::vector<std::vector<double>>& class_vals,
                              std::vector<ds::Fenwick>& fenwicks) const {
    std::vector<std::pair<double, size_t>> remainders(n);
    for (size_t i = 0; i < n; ++i) {
        double remainder = std::fmod(values[i], g1_);
        if (remainder < 0) {
            remainder += g1_;
        }
        remainders[i] = {remainder, i};
    }
    std::sort(remainders.begin(), remainders.end());

    // Assign class IDs to elements based on their sorted order by remainder.
    int cid = 0;
    class_id[remainders[0].second] = cid;
    for (size_t i = 1; i < n; ++i) {
        if (remainders[i].first - remainders[i - 1].first > 1e-9) {
            cid++;
        }
        class_id[remainders[i].second] = cid;
    }

    // Merge boundaries if 0.0 and nearly G1 ended up in separate classes
    // because of the precision error.
    if (cid > 0 && (g1_ - remainders.back().first + remainders[0].first < 1e-9)) {
        int last_cid = class_id[remainders.back().second];
        int first_cid = class_id[remainders[0].second];
        for (size_t i = 0; i < n; ++i) {
            if (class_id[i] == last_cid) {
                class_id[i] = first_cid;
            }
        }
    }

    std::vector<int> mapping(cid + 1, -1);
    int num_classes = 0;
    for (size_t i = 0; i < n; ++i) {
        if (mapping[class_id[i]] == -1) {
            mapping[class_id[i]] = num_classes++;
        }
        class_id[i] = mapping[class_id[i]];
    }

    // Build value lists for each class
    class_vals.resize(num_classes);
    for (size_t i = 0; i < n; ++i) {
        class_vals[class_id[i]].push_back(values[i]);
    }

    for (int c = 0; c < num_classes; ++c) {
        std::sort(class_vals[c].begin(), class_vals[c].end());
        class_vals[c].erase(std::unique(class_vals[c].begin(), class_vals[c].end()),
                            class_vals[c].end());
        fenwicks.emplace_back(class_vals[c].size());
    }
}

// Preprocessing for interval gap case (G1 != G2).
// Extracts unique values and builds segment tree.
void SDVerifier::InitIntervalGap(std::vector<double> const& values,
                                 std::vector<double>& unique_vals,
                                 std::unique_ptr<ds::SegmentTree>& tree,
                                 std::vector<ds::CostNode>& best_prefix_costs) const {
    unique_vals = values;
    std::sort(unique_vals.begin(), unique_vals.end());
    unique_vals.erase(std::unique(unique_vals.begin(), unique_vals.end()), unique_vals.end());

    size_t num_ranks = unique_vals.size();
    tree = std::make_unique<ds::SegmentTree>(num_ranks);
    long const sentinel = std::numeric_limits<long>::max() / 2;
    best_prefix_costs.assign(num_ranks, {sentinel, -1, 0, 0});
}

// Validates predecessor range [l_rank, r_rank) from segment tree.
// If valid path exists, updates min2_cost with the best predecessor.
void SDVerifier::UpdateMin2Interval(double current_val, size_t i, size_t l_rank, size_t r_rank,
                                    std::vector<double> const& unique_vals,
                                    ds::SegmentTree const& tree, long& min2_cost,
                                    long& min2_j) const {
    auto res = tree.Query(l_rank, r_rank);
    if (res.rank == static_cast<size_t>(-1)) {
        return;
    }

    long dcost = CalculateDCost(current_val - unique_vals[res.rank]);
    if (dcost == -1) {
        return;
    }

    long cost = res.t_val + (static_cast<long>(i) - 1 - res.j_val) + (dcost - 1);
    if (min2_cost == -1 || cost <= min2_cost) {
        min2_cost = cost;
        min2_j = res.source_idx;
    }
}

// Evaluates valid predecessor bands backwards from current_val.
// Optimizes by checking only valid bands and stopping when they merge.
void SDVerifier::ProcessIntervalBands(double current_val, size_t i,
                                      std::vector<double> const& unique_vals,
                                      ds::SegmentTree const& tree, long& min2_cost,
                                      long& min2_j) const {
    if (g2_ < 0) {
        double limit = current_val - g1_;
        auto it = std::upper_bound(unique_vals.begin(), unique_vals.end(), limit);
        if (it != unique_vals.begin()) {
            size_t r = std::distance(unique_vals.begin(), it);
            UpdateMin2Interval(current_val, i, 0, r, unique_vals, tree, min2_cost, min2_j);
        }
        return;
    }

    // Zero minimum gap - can chain equal elements
    // All elements <= current_val are valid predecessors (distance = 0)
    if (g1_ < 1e-9) {
        auto it_high = std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val);
        if (it_high != unique_vals.begin()) {
            size_t r = std::distance(unique_vals.begin(), it_high);
            UpdateMin2Interval(current_val, i, 0, r, unique_vals, tree, min2_cost, min2_j);
        }
        return;
    }

    // General interval gap [G1, G2] - iterate over step multipliers k
    // For k steps
    for (long k = 1;; ++k) {
        double high_val = current_val - k * g1_;
        double low_val = current_val - k * g2_;
        // Stop if we've gone past all elements
        if (high_val < unique_vals.front()) {
            break;
        }

        auto it_low = std::lower_bound(unique_vals.begin(), unique_vals.end(), low_val);
        auto it_high = std::upper_bound(unique_vals.begin(), unique_vals.end(), high_val);
        if (it_low < it_high) {
            size_t l_rank = std::distance(unique_vals.begin(), it_low);
            size_t r_rank = std::distance(unique_vals.begin(), it_high);
            UpdateMin2Interval(current_val, i, l_rank, r_rank, unique_vals, tree, min2_cost,
                               min2_j);
        }

        // When k*(G2-G1) >= G1, the ranges start overlapping and cover everything
        if (k * (g2_ - g1_) >= g1_) {
            auto it_rest = std::upper_bound(unique_vals.begin(), unique_vals.end(), high_val);
            if (it_rest != unique_vals.begin()) {
                size_t r_rank = std::distance(unique_vals.begin(), it_rest);
                UpdateMin2Interval(current_val, i, 0, r_rank, unique_vals, tree, min2_cost, min2_j);
            }
            break;
        }
    }
}

// Backtracks from last element to first, building violation list.
// Distinguishes between deletions and insertions.
void SDVerifier::ReconstructPath(std::vector<double> const& values,
                                 std::vector<size_t> const& original_indices,
                                 std::vector<bool> const& ops_from_t,
                                 std::vector<long> const& t_prev, bool is_exact_gap) {
    violations_.clear();
    long current_idx = static_cast<long>(values.size()) - 1;

    // Skip suffix of deleted elements
    while (current_idx >= 0 && !ops_from_t[current_idx]) {
        violations_.push_back(SDDeletion{original_indices[current_idx]});
        --current_idx;
    }

    while (current_idx >= 0) {
        long previous_idx = t_prev[current_idx];
        // No predecessor means all remaining elements were deleted
        if (previous_idx == -1) {
            for (long k = current_idx - 1; k >= 0; --k) {
                violations_.push_back(SDDeletion{original_indices[k]});
            }
            break;
        } else {
            long dcost;
            if (is_exact_gap) {
                dcost = std::lround((values[current_idx] - values[previous_idx]) / g1_);
            } else {
                dcost = CalculateDCost(values[current_idx] - values[previous_idx]);
            }

            // Add insertion violation if more than 1 step is needed
            if (dcost > 1) {
                long min_ins = dcost - 1;
                long max_ins = dcost - 1;
                if (!is_exact_gap && g1_ > 1e-9) {
                    double d = values[current_idx] - values[previous_idx];
                    max_ins = static_cast<long>(std::floor(d / g1_)) - 1;
                }
                violations_.push_back(
                        SDInsertion{original_indices[previous_idx], original_indices[current_idx],
                                    values[previous_idx], values[current_idx], min_ins, max_ins});
            }

            for (long k = current_idx - 1; k > previous_idx; --k) {
                violations_.push_back(SDDeletion{original_indices[k]});
            }

            current_idx = previous_idx;
        }
    }

    std::reverse(violations_.begin(), violations_.end());
}

// Main DP algorithm - finds minimum deletions + insertions.
long SDVerifier::CalculateOps(std::vector<double> const& values,
                              std::vector<size_t> const& original_indices) {
    size_t n = values.size();
    if (n <= 1) {
        return 0;
    }

    bool is_exact_gap = (g2_ > 0 && g2_ - g1_ < 1e-9);

    std::vector<int> class_id(n, 0);
    std::vector<std::vector<double>> class_vals;
    std::vector<ds::Fenwick> fenwicks;
    std::vector<double> unique_vals;
    std::unique_ptr<ds::SegmentTree> tree;
    std::vector<ds::CostNode> best_prefix_costs;

    if (is_exact_gap) {
        InitExactGap(n, values, class_id, class_vals, fenwicks);
    } else {
        InitIntervalGap(values, unique_vals, tree, best_prefix_costs);
    }

    // DP arrays:
    // t[i] = minimum cost reaching element i via optimal path from T
    // ops[i] = minimum operations (deletions + insertions) for prefix [0, i]
    // t_prev[i] = predecessor index of element i in optimal T path
    // ops_from_t[i] = true if OPS[i] came from T[i], false if from OPS[i-1] + 1 deletion
    std::vector<long> t(n, 0);
    std::vector<long> ops(n, 0);
    std::vector<long> t_prev(n, -1);
    std::vector<bool> ops_from_t(n, true);

    // Seed first element: cost = 0, position = 0
    if (is_exact_gap) {
        int c0 = class_id[0];
        auto it = std::lower_bound(class_vals[c0].begin(), class_vals[c0].end(), values[0]);
        size_t rank_0 = std::distance(class_vals[c0].begin(), it);
        fenwicks[c0].Update(rank_0, -std::lround(values[0] / g1_), 0, 0, 0);
    } else {
        auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), values[0]);
        size_t rank0 = std::distance(unique_vals.begin(), it);
        best_prefix_costs[rank0] = {0, 0, 0, 0};
        double val0 = (g2_ < 0) ? 0.0 : (0.0 - unique_vals[rank0] / g2_);
        tree->Update(rank0, val0, rank0, 0, 0, 0);
    }

    for (size_t i = 1; i < n; ++i) {
        long min1_cost = i;   // delete all elements before i
        long min2_cost = -1;  // come from T path
        long min2_j = -1;
        long min3_cost = -1;  // zero gap
        long min3_j = -1;
        double current_val = values[i];

        // Case for exact gap using Fenwick tree
        // Query predecessors in same equivalence class
        if (is_exact_gap) {
            int c = class_id[i];
            auto it = std::lower_bound(class_vals[c].begin(), class_vals[c].end(), current_val);
            size_t rank_i = std::distance(class_vals[c].begin(), it);
            if (rank_i > 0) {
                auto min_res = fenwicks[c].Query(rank_i - 1);
                if (min_res.cost < std::numeric_limits<long>::max() / 2 &&
                    min_res.source_idx != -1) {
                    double prev_val = values[min_res.j_val];
                    long dcost = std::lround((current_val - prev_val) / g1_);
                    long cost =
                            min_res.t_val + static_cast<long>(i) - 1 - min_res.j_val + (dcost - 1);
                    min2_cost = cost;
                    min2_j = min_res.source_idx;
                }
            }
        } else {
            ProcessIntervalBands(current_val, i, unique_vals, *tree, min2_cost, min2_j);
        }

        // Zero gap: can chain equal elements without insertions
        if (!is_exact_gap && g1_ < 1e-9) {
            auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val);
            size_t rank = std::distance(unique_vals.begin(), it);
            if (best_prefix_costs[rank].cost != std::numeric_limits<long>::max() / 2) {
                min3_cost = best_prefix_costs[rank].cost + (i - 1);
                min3_j = best_prefix_costs[rank].source_idx;
            }
        }

        // Choose best path: delete everything, T path, or zero gap
        t[i] = min1_cost;
        t_prev[i] = -1;
        if (min2_cost != -1 && min2_cost <= t[i]) {
            t[i] = min2_cost;
            t_prev[i] = min2_j;
        }
        if (g1_ < 1e-9 && min3_cost != -1 && min3_cost <= t[i]) {
            t[i] = min3_cost;
            t_prev[i] = min3_j;
        }

        // Either follow T path, or delete current element
        if (ops[i - 1] + 1 < t[i]) {
            ops[i] = ops[i - 1] + 1;
            ops_from_t[i] = false;
        } else {
            ops[i] = t[i];
            ops_from_t[i] = true;
        }

        // Insert current element into data structures
        if (is_exact_gap) {
            int c = class_id[i];
            auto it = std::lower_bound(class_vals[c].begin(), class_vals[c].end(), current_val);
            size_t rank_i = std::distance(class_vals[c].begin(), it);
            long v_i = t[i] - static_cast<long>(i) -
                       std::lround(current_val / g1_);  // key = T(i) - i - A[i]/G
            fenwicks[c].Update(rank_i, v_i, static_cast<long>(i), t[i], static_cast<long>(i));
        } else {
            auto it = std::lower_bound(unique_vals.begin(), unique_vals.end(), current_val);
            size_t rank_i = std::distance(unique_vals.begin(), it);
            long t_minus_i = t[i] - static_cast<long>(i);
            if (t_minus_i < best_prefix_costs[rank_i].cost) {
                best_prefix_costs[rank_i] = {t_minus_i, static_cast<long>(i), t[i],
                                             static_cast<long>(i)};
                double val_i = (g2_ < 0) ? static_cast<double>(t_minus_i)
                                         : (static_cast<double>(t_minus_i) - current_val / g2_);
                tree->Update(rank_i, val_i, rank_i, static_cast<long>(i), t[i],
                             static_cast<long>(i));
            }
        }
    }

    ReconstructPath(values, original_indices, ops_from_t, t_prev, is_exact_gap);
    return ops[n - 1];
}

unsigned long long SDVerifier::ExecuteInternal() {
    auto start_time = std::chrono::steady_clock::now();

    if (lhs_indices_.size() != 1 || rhs_indices_.size() != 1) {
        throw std::runtime_error(
                "SDVerifier currently supports exactly one lhs and one rhs column.");
    }
    if (g1_ < 0) {
        throw std::runtime_error("g1 must be non-negative.");
    }
    if (g2_ >= 0 && g2_ < g1_) {
        throw std::runtime_error("g2 must be >= g1 or negative.");
    }
    if (g2_ >= 0 && g2_ < 1e-9 && g1_ < 1e-9) {
        throw std::runtime_error("g1 and g2 cannot both be zero.");
    }

    x_val_.clear();
    y_val_.clear();
    std::vector<size_t> selected_row_ids;
    size_t lhs_idx = lhs_indices_[0];
    size_t rhs_idx = rhs_indices_[0];
    bool use_subset = !indices_.empty();

    if (use_subset) {
        for (auto row_idx : indices_) {
            if (row_idx >= raw_data_.size()) {
                throw std::runtime_error(
                        "Subset row index is out of bounds: " + std::to_string(row_idx) + ".");
            }
        }
    }

    std::unordered_set<size_t> target_indices(indices_.begin(), indices_.end());

    auto parse_numeric = [](std::string const& raw) {
        size_t parsed = 0;
        double value = 0.0;
        try {
            value = std::stod(raw, &parsed);
        } catch (std::exception const&) {
            throw std::runtime_error("Failed to parse numeric value.");
        }
        for (size_t i = parsed; i < raw.size(); ++i) {
            if (!std::isspace(static_cast<unsigned char>(raw[i]))) {
                throw std::runtime_error("Failed to parse numeric value.");
            }
        }
        return value;
    };

    for (size_t i = 0; i < raw_data_.size(); ++i) {
        auto const& row = raw_data_[i];
        if (row.size() <= std::max(lhs_idx, rhs_idx)) {
            continue;
        }
        if (use_subset && target_indices.find(i) == target_indices.end()) {
            continue;
        }
        x_val_.push_back(parse_numeric(row[lhs_idx]));
        y_val_.push_back(parse_numeric(row[rhs_idx]));
        selected_row_ids.push_back(i);
    }

    size_t rows = x_val_.size();
    if (rows <= 1) {
        ops_ = 0;
        confidence_ = 1.0;
        violations_.clear();
        auto end_time = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    }

    std::vector<size_t> p(rows);
    std::iota(p.begin(), p.end(), 0);
    std::sort(p.begin(), p.end(), [&](size_t a, size_t b) {
        if (x_val_[a] != x_val_[b]) {
            return x_val_[a] < x_val_[b];
        } else if (y_val_[a] != y_val_[b]) {
            return y_val_[a] < y_val_[b];
        }
        return a < b;
    });

    std::vector<double> values(rows);
    std::vector<size_t> original_indices(rows);
    for (size_t i = 0; i < rows; ++i) {
        values[i] = y_val_[p[i]];
        original_indices[i] = selected_row_ids[p[i]];
    }

    ops_ = CalculateOps(values, original_indices);
    confidence_ = static_cast<double>(rows - ops_) / rows;

    auto end_time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
}

}  // namespace algos::sd_verifier
