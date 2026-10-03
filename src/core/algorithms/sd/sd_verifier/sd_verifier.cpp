#include "sd_verifier.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "core/algorithms/sd/util/numeric_utils.h"
#include "core/config/column_index/option.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace {
bool AlmostEqual(double A, double B,
                 double maxRelDiff = std::numeric_limits<double>::epsilon() * 100.0,
                 double maxAbsDiff = std::numeric_limits<double>::epsilon() * 100.0) {
    double diff = std::abs(A - B);
    if (diff <= maxAbsDiff) {
        return true;
    }

    A = std::abs(A);
    B = std::abs(B);
    double largest = std::max(A, B);
    return diff <= largest * maxRelDiff;
}
}  // namespace

namespace algos::sd_verifier {

SDVerifier::SDVerifier() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void SDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName(),
                          kSdIndices, kSdG1});
}

void SDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto check_positive = [](double val) {
        if (val < 0) {
            throw std::runtime_error("g1 must be non-negative.");
        }
    };
    auto check_g2 = [this](double val) {
        if (val >= 0.0 && val < g1_) {
            throw std::runtime_error("g2 must be >= g1 or negative.");
        }
        if (val >= 0.0 && AlmostEqual(val, 0.0) && AlmostEqual(g1_, 0.0)) {
            throw std::runtime_error("g1 and g2 cannot both be zero.");
        }
    };

    auto check_single_col = [](config::IndicesType const& val) {
        if (val.size() != 1) {
            throw std::runtime_error(
                    "SDVerifier currently supports exactly one lhs and one rhs column.");
        }
    };
    auto check_indices = [this](config::IndicesType const& val) {
        for (auto row_idx : val) {
            if (row_idx >= raw_data_.size()) {
                throw std::runtime_error(
                        "Subset row index is out of bounds: " + std::to_string(row_idx) + ".");
            }
        }
    };

    RegisterOption(config::kTableOpt(&input_table_));

    auto get_cols_num = [this]() {
        return input_table_ ? static_cast<config::IndexType>(input_table_->GetNumberOfColumns())
                            : 0;
    };

    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_cols_num, check_single_col));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_cols_num, check_single_col));
    RegisterOption(Option<config::IndicesType>{
            &indices_, kSdIndices, kDSdIndices, []() {
                return config::IndicesType{};
            }}.SetValueCheck(check_indices));
    RegisterOption(Option<double>{&g1_, kSdG1, kDSdG1}
                           .SetValueCheck(check_positive)
                           .SetConditionalOpts({{{}, {kSdG2}}}));
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

// Backtracks from last element to first, building violation list.
// Distinguishes between deletions and insertions.
void SDVerifier::ReconstructPath(std::vector<double> const& values,
                                 std::vector<size_t> const& original_indices,
                                 algos::sd::util::EditDistanceTrace const& trace,
                                 algos::sd::util::EditDistanceCalculator const& calculator) {
    violations_.clear();
    if (values.empty()) {
        return;
    }

    long current_idx = static_cast<long>(values.size()) - 1;

    // Skip suffix of deleted elements
    while (current_idx >= 0 && !trace.ops_from_t[current_idx]) {
        violations_.push_back(SDDeletion{original_indices[current_idx]});
        --current_idx;
    }

    while (current_idx >= 0) {
        long previous_idx = trace.t_prev[current_idx];
        // No predecessor means all remaining elements were deleted
        if (previous_idx == -1) {
            for (long k = current_idx - 1; k >= 0; --k) {
                violations_.push_back(SDDeletion{original_indices[k]});
            }
            break;
        } else {
            long double const distance = static_cast<long double>(values[current_idx]) -
                                         static_cast<long double>(values[previous_idx]);
            long const dcost = calculator.CalculateDCost(distance);
            if (dcost < 1) {
                throw std::logic_error("SD edit-distance trace contains an invalid bridge.");
            }

            // Add insertion violation if more than 1 step is needed
            if (dcost > 1) {
                long min_ins = dcost - 1;
                long max_ins = dcost - 1;
                if (!trace.is_exact_gap && !AlmostEqual(g1_, 0.0)) {
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

long SDVerifier::CalculateOps(std::vector<double> const& values,
                              std::vector<size_t> const& original_indices) {
    algos::sd::util::EditDistanceCalculator calculator(g1_, g2_);
    auto trace = calculator.CalculateTrace(values);
    ReconstructPath(values, original_indices, trace, calculator);
    return trace.ops;
}

void SDVerifier::ExecuteInternal() {
    x_val_.clear();
    y_val_.clear();
    std::vector<size_t> selected_row_ids;
    size_t lhs_idx = lhs_indices_[0];
    size_t rhs_idx = rhs_indices_[0];
    bool use_subset = !indices_.empty();

    std::unordered_set<size_t> target_indices(indices_.begin(), indices_.end());

    for (size_t i = 0; i < raw_data_.size(); ++i) {
        auto const& row = raw_data_[i];
        if (row.size() <= std::max(lhs_idx, rhs_idx)) {
            continue;
        }
        if (use_subset && target_indices.find(i) == target_indices.end()) {
            continue;
        }
        x_val_.push_back(sd::util::ParseNumeric(row[lhs_idx]));
        y_val_.push_back(sd::util::ParseNumeric(row[rhs_idx]));
        selected_row_ids.push_back(i);
    }

    size_t rows = x_val_.size();
    if (rows <= 1) {
        ops_ = 0;
        confidence_ = 1.0;
        violations_.clear();
        return;
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
}

}  // namespace algos::sd_verifier
