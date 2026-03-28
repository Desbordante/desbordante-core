#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/algorithms/pac/fd_pac.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/column_metric.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/tuple_pair.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/util/make_tuples.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/model/types/type.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {
// FD PAC X -> Y specifies that
//  if |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X
//  then Pr(|t_i[B_l] - t_j[B_l]| <= eps_l) >= delta for each B_l in Y
//
// NOTE: PAC-Man based algorithms (including this one) can handle only PACs that have
// eps_1 = ... = eps_n = eps
//
// Key steps of the algorithm:
// 1. Select subset Gamma of r^2:
//     Gamma = {(t_i, t_j) : |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X}
// 3. Sort Gamma by max|t_i[B_l] - t_j[B_l]|
// 4. For each gamma(n) find the greatest eps(n), such that epm_prob(n) <= delta(n), where
//     sigma(n) = {(t_i, t_j) in Gamma : |t_i[B_l] - t_j[B_l]| <= eps(n) for each B_l in Y}
//	   emp_prob(n) = |sigma(n)| / |Gamma|
//    This step is done by repeatedly "widening" sigma(n). See Domain PAC verifier for a more
//    straightforward usage of this approach
//
// Optimizations and other notes:
// 1. For each pair (t_i, t_j) in Gamma select l0, l1 such that
//     |t_i[A_l0] - t_j[A_l0]| = max|t_i[A_l] - t_j[A_l]| for each A_l in X
//     |t_i[B_l1] - t_j[B_l1]| = max|t_i[B_l] - t_j[B_l]| for each B_l in Y
//    It's obvious that only A_l0 and B_l1 must be considered.
// 2. Only subset Gamma' of Gamma needs to be considered:
//     Gamma' = {(t_i, t_j) in Gamma : i < j},  Gamma'' = {(t_i, t_j) in Gamma : i > j}
//     sigma' = sigma \cap Gamma',  sigma'' = sigma \cap Gamma''
//    Consider a subset D of r^2:
//     D = {(t_i, t_i) in r^2}
//    It's obvious that
//     |D| = |r|,  Gamma = Gamma' + D + Gamma'',  sigma = sigma' + D + sigma''
//     |Gamma'| = |Gamma''|,  |sigma'| = |sigma''|
//    Thus, empirical probability becomes
//     emp_prob = (2|sigma'(n)| + |r|) / (2|Gamma'| + |r|)

/// @brief Functional Probabilistic Approximate Constraints verifier.
/// FDPACVerifier<true> is C++/Python version, which takes @c vectors of @c std::functions as
/// metrics. FDPACVerifier<false> is CLI version, which always uses default metrics.
template <bool MetricOpt = true>
class FDPACVerifier : public PACVerifier {
private:
    using Pairs = std::vector<TuplePair>;
    using PairsIt = Pairs::const_iterator;

    constexpr static double kDefaultLhsDelta = kDistThreshold;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    std::shared_ptr<std::vector<model::Type const*>> lhs_types_;
    std::shared_ptr<std::vector<model::Type const*>> rhs_types_;

    std::vector<ValueMetric> lhs_metrics_opt_;
    std::vector<ValueMetric> rhs_metrics_opt_;

    std::vector<ColumnMetric> lhs_metrics_;
    std::vector<ColumnMetric> rhs_metrics_;

    std::vector<double> lhs_Deltas_;

    std::shared_ptr<pac::model::Tuples> lhs_tuples_;
    std::shared_ptr<pac::model::Tuples> rhs_tuples_;

    // Gamma is a set of pairs such that |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X.
    // This vector holds a subset Gamma' of Gamma (see notes), sorted by max{|t_i[B_l] - t_j[B_l]}
    std::shared_ptr<Pairs> sorted_gamma_;

    /// @brief Fill sorted_gamma. Called in Execute, because Deltas is an execute option.
    void PreparePairs();

    /// @brief For each delta_i find the least eps_i such that PAC_{eps_i}^{delta_i} holds.
    /// Then refine delta_i, i. e. find the greatest delta_i' such that PAC_{eps_i}^{delta_i} holds.
    /// @return (eps_i, delta_i') pairs
    std::vector<std::pair<double, double>> CalculateEmpiricalProbabilities() const;

    void ProcessPACTypeOptions() override;
    void PreparePACTypeData() override;
    void PACTypeExecuteInternal() override;

    void MakeExecuteOptsAvailable() override {
        PACVerifier::MakeExecuteOptsAvailable();
        MakeOptionsAvailable({config::names::kLhsDeltas});
    }

public:
    FDPACVerifier();

    FDPACHighlight GetHighlights(double eps_1 = 0, double eps_2 = -1) const;
};

template <bool MetricOpt>
void FDPACVerifier<MetricOpt>::PreparePairs() {
    auto col_dist = [](std::vector<ColumnMetric> const& metrics, pac::model::Tuple const& a,
                       pac::model::Tuple const& b, std::size_t col_num) -> double {
        return metrics[col_num].Dist(a[col_num], b[col_num]);
    };

    // All pairs have first_idx < second_idx. See "key ideas".
    sorted_gamma_ = std::make_shared<Pairs>();
    auto const num_rows = TypedRelation().GetNumRows();
    for (std::size_t i = 0; i < num_rows; ++i) {
        for (std::size_t j = i + 1; j < num_rows; ++j) {
            auto const& first_lhs = (*lhs_tuples_)[i];
            auto const& second_lhs = (*lhs_tuples_)[j];
            bool add_to_gamma = true;
            for (std::size_t col_num = 0; col_num < first_lhs.size(); ++col_num) {
                auto lhs_dist = col_dist(lhs_metrics_, first_lhs, second_lhs, col_num);
                if (lhs_dist > lhs_Deltas_[col_num]) {
                    add_to_gamma = false;
                    break;
                }
            }
            if (add_to_gamma) {
                auto const& first_rhs = (*rhs_tuples_)[i];
                auto const& second_rhs = (*rhs_tuples_)[j];
                double max_rhs_dist = 0;
                for (std::size_t col_num = 0; col_num < first_rhs.size(); ++col_num) {
                    max_rhs_dist = std::max(max_rhs_dist,
                                            col_dist(rhs_metrics_, first_rhs, second_rhs, col_num));
                }
                sorted_gamma_->push_back(TuplePair{i, j, max_rhs_dist});
            }
        }
    }
    std::ranges::sort(*sorted_gamma_, {}, [](TuplePair const& p) { return p.rhs_dist; });

    LOG_TRACE("Sorted gamma:");
    for ([[maybe_unused]] TuplePair const& pair : *sorted_gamma_) {
        LOG_TRACE("\tFirst: {}, second: {}, rhs_dist: {}", pair.first_idx, pair.second_idx,
                  pair.rhs_dist);
    }
}

template <bool MetricOpt>
std::vector<std::pair<double, double>> FDPACVerifier<MetricOpt>::CalculateEmpiricalProbabilities()
        const {
    // NOTE: A 10^6-row table (which is an ordinary case) will contain 10^12 pairs.
    // But if all these TuplePair objects fit in memory, std::size_t will be guaranteed to
    // be large enough to hold their number
    std::size_t total_tuples = TypedRelation().GetNumRows();
    std::size_t total_pairs = total_tuples * total_tuples;

    std::vector<std::pair<double, double>> result;

    if (sorted_gamma_->empty()) {
        return {{0, 0}, {0, 1}};
    }

    // delta = (2 * pairs + total_tuples) / (2 * |sorted_gamma_| + total_tuples)
    // => pairs = (delta * (2 * |sorted_gamma_| + total_tuples) - total_tuples) / 2 =
    //           = delta * |sorted_gamma_| + total_tuples * (delta - 1) / 2
    auto get_num_pairs = [total_tuples, gamma_sz = sorted_gamma_->size()](double delta) -> double {
        double num_pairs = delta * gamma_sz + (delta - 1) * total_tuples / 2;
        // get_delta(0) > 0 when total_tuples > 0  =>
        //     exists some minimal delta > 0 such that num_pairs(delta) = 0
        if (num_pairs < 0) {
            num_pairs = 0;
        }
        return num_pairs;
    };
    auto get_delta = [total_tuples,
                      gamma_sz = sorted_gamma_->size()](std::size_t pairs_num) -> double {
        double delta =
                static_cast<double>(2 * pairs_num + total_tuples) / (2 * gamma_sz + total_tuples);
        LOG_TRACE("Delta for {} pairs: {} = (2 * {} + {}) / (2 * {} + {})", pairs_num, delta,
                  pairs_num, total_tuples, gamma_sz, total_tuples);
        assert(delta >= -PACVerifier::kDistThreshold && delta <= 1 + PACVerifier::kDistThreshold);
        return delta;
    };

    // Use ceil to ensure that min_pairs is always enough to satisfy min_delta
    std::size_t min_pairs_num = std::ceil(get_num_pairs(MinDelta()));
    LOG_TRACE("Min pairs num: {} = {} * {} + {} * {} / 2", min_pairs_num, MinDelta(),
              sorted_gamma_->size(), MinDelta() - 1, total_tuples);
    assert(min_pairs_num <= total_pairs);

    std::size_t pairs_step;
    if (DeltaSteps() <= 1) {
        pairs_step = total_pairs - min_pairs_num;
    } else {
        pairs_step =
                std::round(static_cast<double>(total_pairs - min_pairs_num) / (DeltaSteps() - 1));
    }
    if (pairs_step == 0) {
        LOG_DEBUG("Pairs step is 0. Setting to 1");
        pairs_step = 1;
    }

    auto end = std::ranges::upper_bound(*sorted_gamma_, 0, {},
                                        [](TuplePair const& p) { return p.rhs_dist; });
    std::size_t curr_size = std::distance(sorted_gamma_->begin(), end);
    result.emplace_back(0, get_delta(curr_size));
    LOG_DEBUG("Delta steps: {}, pairs step: {}, initial size: {} (delta: {})", DeltaSteps(),
              pairs_step, curr_size, get_delta(curr_size));

    if (sorted_gamma_->empty()) {
        result.emplace_back(0, 1);
        return result;
    }

    auto iteration = [&](std::size_t needed_pairs_num) {
        // Find eps_i
        auto need_to_add = needed_pairs_num - curr_size;
        auto actually_add = std::min(
                need_to_add, static_cast<std::size_t>(std::distance(end, sorted_gamma_->end())));
        std::advance(end, actually_add);
        curr_size += actually_add;
        // end != sorted_gamma_->begin() here, because
        //  a) we've checked that sorted_gamma_ is not empty
        //  b) we've checked that needed_pairs_num > curr_size (which is at least 0)
        assert(end != sorted_gamma_->begin());
        auto eps_i = std::prev(end)->rhs_dist;
        LOG_TRACE("Eps for {} pairs: {}", needed_pairs_num, eps_i);

        // Refine delta_i
        while (end != sorted_gamma_->end() && end->rhs_dist - eps_i < kDistThreshold) {
            std::advance(end, 1);
            ++curr_size;
        }
        assert(curr_size == static_cast<std::size_t>(std::distance(sorted_gamma_->begin(), end)));
        LOG_TRACE("Refined size: {}", curr_size);
        result.emplace_back(eps_i, get_delta(curr_size));
    };
    for (auto needed_pairs_num = min_pairs_num; needed_pairs_num < sorted_gamma_->size();
         needed_pairs_num += pairs_step) {
        if (needed_pairs_num <= curr_size) {
            continue;
        }
        iteration(needed_pairs_num);
    }
    // Ensure that (??, 1) is always in empirical_probabilities
    iteration(sorted_gamma_->size());

    return result;
}

template <bool MetricOpt>
void FDPACVerifier<MetricOpt>::ProcessPACTypeOptions() {
    auto check_indices = [num_cols = TypedRelation().GetNumColumns()](auto const& indices) {
        auto max_idx = std::ranges::max(indices);
        if (max_idx >= num_cols) {
            throw config::ConfigurationError("Column index " + std::to_string(max_idx) +
                                             " out of range");
        }
    };
    check_indices(lhs_indices_);
    check_indices(rhs_indices_);

    auto const& col_data = TypedRelation().GetColumnData();
    auto process_options = [&col_data, dist_from_null_is_infty = DistFromNullIsInfty()](
                                   auto& types, auto& metrics_opt, auto const& indices) {
        types = std::make_shared<std::vector<model::Type const*>>(indices.size());
        std::ranges::transform(indices, types->begin(), [&col_data](std::size_t const idx) {
            return &col_data[idx].GetType();
        });

        if (metrics_opt.size() > indices.size()) {
            throw config::ConfigurationError(
                    "Number of used metrics must be less or equal to the number of indices");
        }
        metrics_opt.resize(indices.size());
        std::vector<ColumnMetric> metrics;
        metrics.reserve(indices.size());
        std::ranges::transform(
                *types, metrics_opt, std::back_inserter(metrics),
                [dist_from_null_is_infty](model::Type const* type, ValueMetric v_metric) {
                    return std::visit(
                            [&type, dist_from_null_is_infty](auto&& arg) mutable {
                                return ColumnMetric{type, std::move(arg), dist_from_null_is_infty};
                            },
                            v_metric);
                });
        return metrics;
    };

    lhs_metrics_ = process_options(lhs_types_, lhs_metrics_opt_, lhs_indices_);
    rhs_metrics_ = process_options(rhs_types_, rhs_metrics_opt_, rhs_indices_);
}

template <bool MetricOpt>
void FDPACVerifier<MetricOpt>::PreparePACTypeData() {
    using namespace pac::util;

    lhs_tuples_ = MakeTuples(TypedRelation().GetColumnData(), lhs_indices_);
    rhs_tuples_ = MakeTuples(TypedRelation().GetColumnData(), rhs_indices_);
}

template <bool MetricOpt>
void FDPACVerifier<MetricOpt>::PACTypeExecuteInternal() {
    auto vec_to_str = [](auto const& vec) -> std::string {
        std::ostringstream oss;
        oss << '{';
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it != vec.begin()) {
                oss << ", ";
            }
            oss << *it;
        }
        oss << '}';
        return oss.str();
    };
    LOG_INFO("Verifying FD PAC {} -> {} with LHS Deltas = {}", vec_to_str(lhs_indices_),
             vec_to_str(rhs_indices_), vec_to_str(lhs_Deltas_));

    PreparePairs();

    auto emp_probabilities = CalculateEmpiricalProbabilities();
    auto [epsilon, delta] = FindEpsilonDelta(std::move(emp_probabilities));
    // By definition, FD PAC may have different epsilons for each RHS column, but this algo cannot
    // verify such FD PACs
    std::vector<double> epsilons(rhs_indices_.size(), epsilon);

    auto make_vertical = [this](config::IndicesType const& indices) {
        return TypedRelation().GetSchema()->GetVertical(util::IndicesToBitset(
                indices.begin(), indices.end(), TypedRelation().GetNumColumns()));
    };
    MakePAC<model::FDPAC>(TypedRelation().GetSharedPtrSchema(), make_vertical(lhs_indices_),
                          make_vertical(rhs_indices_), std::move(lhs_Deltas_), std::move(epsilons),
                          delta);

    LOG_INFO("Result: {}", GetPAC().ToLongString());
}

template <bool MetricOpt>
FDPACVerifier<MetricOpt>::FDPACVerifier() : PACVerifier() {
    DESBORDANTE_OPTION_USING;

    // Load data options
    RegisterOption(Option(&lhs_indices_, kLhsIndices, kDLhsIndices));
    RegisterOption(Option(&rhs_indices_, kRhsIndices, kDRhsIndices));
    if constexpr (MetricOpt) {
        RegisterOption(
                Option(&lhs_metrics_opt_, kLhsMetrics, kDLhsMetrics, std::vector<ValueMetric>{}));
        RegisterOption(
                Option(&rhs_metrics_opt_, kRhsMetrics, kDRhsMetrics, std::vector<ValueMetric>{}));
    }

    // Execute options
    RegisterOption(Option(&lhs_Deltas_, kLhsDeltas, kDLhsDeltas, std::vector<double>{})
                           .SetValueCheck([this](std::vector<double> const& value) {
                               if (value.size() > lhs_indices_.size()) {
                                   throw config::ConfigurationError("Too many LHS deltas");
                               }
                           })
                           .SetNormalizeFunc([this](std::vector<double>& value) {
                               if (value.empty()) {
                                   value.push_back(kDefaultLhsDelta);
                               }
                               value.resize(lhs_indices_.size(), value.back());
                           }));

    MakeOptionsAvailable({kLhsIndices, kRhsIndices});
    if constexpr (MetricOpt) {
        MakeOptionsAvailable({kLhsMetrics, kRhsMetrics});
    }
}

template <bool MetricOpt>
FDPACHighlight FDPACVerifier<MetricOpt>::GetHighlights(double eps_1, double eps_2) const {
    if (eps_2 < 0) {
        auto const& epsilons = GetPAC().GetEpsilons();
        if (epsilons.empty()) {
            throw std::runtime_error("Execute must be called before calling GetHihglights");
        }
        eps_2 = epsilons.front();
    }
    if (eps_2 <= eps_1) {
        return FDPACHighlight{};
    }
    LOG_DEBUG("Calculating higlights from {} to {}...", eps_1, eps_2);

    auto begin = std::ranges::upper_bound(*sorted_gamma_, eps_1, {},
                                          [](TuplePair const& pair) { return pair.rhs_dist; });
    auto end = std::ranges::upper_bound(begin, sorted_gamma_->end(), eps_2, {},
                                        [](TuplePair const& pair) { return pair.rhs_dist; });

    LOG_DEBUG("Highlighted pairs [{}, {})", std::distance(sorted_gamma_->begin(), begin),
              std::distance(sorted_gamma_->begin(), end));
    return FDPACHighlight{lhs_tuples_,   rhs_tuples_, lhs_types_, rhs_types_,
                          sorted_gamma_, begin,       end};
}
}  // namespace algos::pac_verifier
