#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "core/algorithms/pac/fd_pac.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/pac_verifier/util/make_tuples.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/config/custom_metric/custom_metrics_option.h"
#include "core/config/descriptions.h"
#include "core/config/indices/option.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {
double FDPACVerifier::GetNumPairs(double delta) const {
    // delta = (2 * pairs + total_tuples) / (2 * |sorted_gamma_| + total_tuples)
    // => pairs = (delta * (2 * |sorted_gamma_| + total_tuples) - total_tuples) / 2 =
    //          = delta * |sorted_gamma_| + total_tuples * (delta - 1) / 2
    double num_pairs =
            delta * sorted_gamma_->size() + (delta - 1) * TypedRelation().GetNumRows() / 2;
    LOG_TRACE("Num pairs for delta = {}: {} = {} * {} + {} * ({} - 1) / 2", delta, num_pairs, delta,
              sorted_gamma_->size(), delta, TypedRelation().GetNumRows());
    // get_delta(0) > 0 when total_tuples > 0  =>
    //     exists some minimal delta > 0 such that num_pairs(delta) = 0
    if (num_pairs < 0) {
        num_pairs = 0;
    }
    return num_pairs;
}

double FDPACVerifier::GetDelta(std::size_t num_pairs) const {
    auto const total_tuples = TypedRelation().GetNumRows();
    auto const gamma_size = sorted_gamma_->size();

    double delta =
            static_cast<double>(2 * num_pairs + total_tuples) / (2 * gamma_size + total_tuples);
    LOG_TRACE("Delta for {} pairs: {} = (2 * {} + {}) / (2 * {} + {})", num_pairs, delta, num_pairs,
              total_tuples, gamma_size, total_tuples);
    assert(delta >= -kDistThreshold && delta <= 1 + kDistThreshold);
    return delta;
}

void FDPACVerifier::PreparePairs() {
    auto col_dist = [](Metrics const& metrics, Types const& types, pac::model::Tuple const& a,
                       pac::model::Tuple const& b, std::size_t col_num) -> double {
        return metrics[col_num]->Dist(types[col_num], a[col_num], b[col_num]);
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
                auto lhs_dist = col_dist(lhs_metrics_, *lhs_types_, first_lhs, second_lhs, col_num);
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
                    max_rhs_dist = std::max(max_rhs_dist, col_dist(rhs_metrics_, *rhs_types_,
                                                                   first_rhs, second_rhs, col_num));
                }
                sorted_gamma_->push_back(TuplePair{i, j, max_rhs_dist});
            }
        }
    }
    std::ranges::sort(*sorted_gamma_, {}, [](TuplePair const& p) { return p.dist; });
}

void FDPACVerifier::PreparePACTypeData() {
    using namespace pac::util;

    lhs_tuples_ = MakeTuples(TypedRelation().GetColumnData(), lhs_indices_);
    rhs_tuples_ = MakeTuples(TypedRelation().GetColumnData(), rhs_indices_);

    auto const& col_data = TypedRelation().GetColumnData();
    auto make_types = [&col_data](config::IndicesType const& indices) -> std::shared_ptr<Types> {
        auto types = std::make_shared<Types>(indices.size());
        std::ranges::transform(indices, types->begin(), [&col_data](std::size_t const idx) {
            return &col_data[idx].GetType();
        });
        return types;
    };
    lhs_types_ = make_types(lhs_indices_);
    rhs_types_ = make_types(rhs_indices_);
}

void FDPACVerifier::PACTypeExecuteInternal() {
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

    auto emp_probabilities = CalculateEmpiricalProbabilities(*sorted_gamma_);
    auto [epsilon, delta] = FindEpsilonDelta(std::move(emp_probabilities));
    // By definition, FD PAC may have different epsilons for each RHS column, but this algo cannot
    // verify such FD PACs
    std::vector<double> epsilons(rhs_indices_.size(), epsilon);

    auto make_vertical = [this](config::IndicesType const& indices) {
        return TypedRelation().GetSchema()->GetVertical(util::IndicesToBitset(
                indices.begin(), indices.end(), TypedRelation().GetNumColumns()));
    };
    pac_ = model::FDPAC{make_vertical(lhs_indices_), make_vertical(rhs_indices_),
                        std::move(lhs_Deltas_), std::move(epsilons), delta};

    LOG_INFO("Result: {}", pac_->ToLongString());
}

void FDPACVerifier::RefineDelta(FDPACVerifier::PairsIt& iter) const {
    auto initial_eps = iter->dist;
    while (iter != sorted_gamma_->end() && iter->dist - initial_eps < kDistThreshold) {
        std::advance(iter, 1);
    }
    std::advance(iter, -1);
}

FDPACVerifier::FDPACVerifier() {
    DESBORDANTE_OPTION_USING;
    using namespace config;

    RegisterOption(config::kTableOpt(&input_table_)
                           .SetConditionalOpts({{nullptr, {kLhsIndices, kRhsIndices}}}));

    // Load data options
    RegisterOption(kLhsRawIndicesOpt(&lhs_indices_, [this]() {
                       return input_table_->GetNumberOfColumns();
                   }).SetConditionalOpts({{nullptr, {kLhsMetrics}}}));
    RegisterOption(kRhsRawIndicesOpt(&rhs_indices_, [this]() {
                       return input_table_->GetNumberOfColumns();
                   }).SetConditionalOpts({{nullptr, {kRhsMetrics}}}));
    RegisterOption(MetricsOption(&lhs_metrics_, kLhsMetrics, kDLhsMetrics)
                           .SetNormalizeFunc([this](auto& value) {
                               MetricsOption::NormalizeMetrics(value, lhs_indices_.size());
                           }));
    RegisterOption(MetricsOption(&rhs_metrics_, kRhsMetrics, kDRhsMetrics)
                           .SetNormalizeFunc([this](auto& value) {
                               MetricsOption::NormalizeMetrics(value, rhs_indices_.size());
                           }));

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

    RegisterOption(Option(&min_delta_, kMinDelta, kDMinDelta, -1.0).SetValueCheck([](double x) {
        return x <= 1;
    }));

    MakeOptionsAvailable({kTableOpt.GetName()});
}

FDPACHighlight FDPACVerifier::GetHighlights(double eps_1, double eps_2) const {
    if (eps_2 < 0) {
        auto const& epsilons = GetPAC().GetEpsilons();
        if (epsilons.empty()) {
            throw std::runtime_error("Execute must be called before calling GetHihghlights");
        }
        eps_2 = epsilons.front();
    }
    if (eps_2 <= eps_1) {
        return FDPACHighlight{};
    }
    LOG_DEBUG("Calculating highlights from {} to {}...", eps_1, eps_2);

    auto begin = std::ranges::upper_bound(*sorted_gamma_, eps_1, {},
                                          [](TuplePair const& pair) { return pair.dist; });
    auto end = std::ranges::upper_bound(begin, sorted_gamma_->end(), eps_2, {},
                                        [](TuplePair const& pair) { return pair.dist; });

    LOG_DEBUG("Highlighted pairs [{}, {})", std::distance(sorted_gamma_->begin(), begin),
              std::distance(sorted_gamma_->begin(), end));
    return FDPACHighlight{lhs_tuples_,   rhs_tuples_, lhs_types_, rhs_types_,
                          sorted_gamma_, begin,       end};
}
}  // namespace algos::pac_verifier
