#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/util/make_tuples.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/algorithms/pac/ucc_pac.h"
#include "core/config/custom_metric/custom_vector_metric_option.h"
#include "core/config/descriptions.h"
#include "core/config/indices/option.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {
double UCCPACVerifier::GetNumPairs(double delta) const {
    // delta = (2 * pairs + total_tuples) / total_tuples^2
    // => pairs = (delta * total_tuples^2 - total_tuples) / 2 =
    //          = total_tuples * (delta * total_tuples - 1) / 2
    double num_pairs = (delta * std::pow(tuples_->size(), 2) - tuples_->size()) / 2;
    if (num_pairs < 0) {
        num_pairs = 0;
    }
    return num_pairs;
}

double UCCPACVerifier::GetDelta(std::size_t num_pairs) const {
    double delta = static_cast<double>(2 * num_pairs + tuples_->size()) /
                   std::pow(sorted_pairs_->size(), 2);
    assert(delta >= -PACVerifier::kDistThreshold && delta <= 1 + PACVerifier::kDistThreshold);
    return delta;
}

void UCCPACVerifier::PreparePairs() {
    sorted_pairs_ = std::make_shared<Pairs>();
    auto const total_tuples = tuples_->size();
    sorted_pairs_->reserve((total_tuples * (total_tuples - 1)) / 2);
    for (std::size_t i = 0; i < total_tuples; ++i) {
        for (std::size_t j = i + 1; j < total_tuples; ++j) {
            auto const& first = (*tuples_)[i];
            auto const& second = (*tuples_)[j];
            sorted_pairs_->emplace_back(i, j,
                                        metric_->Dist(tuple_type_->GetTypes(), first, second));
        }
    }
    std::ranges::sort(*sorted_pairs_, {}, [](TuplePair const& p) { return p.dist; });
}

void UCCPACVerifier::ProcessPACTypeOptions() {
    std::vector<model::Type const*> types(column_indices_.size());
    auto const& col_data = TypedRelation().GetColumnData();
    std::ranges::transform(column_indices_, types.begin(),
                           [&col_data](std::size_t const idx) { return &col_data[idx].GetType(); });

    tuple_type_ = std::make_shared<pac::model::TupleType>(std::move(types));
}

void UCCPACVerifier::PreparePACTypeData() {
    tuples_ = pac::util::MakeTuples(TypedRelation().GetColumnData(), column_indices_);
}

void UCCPACVerifier::PACTypeExecuteInternal() {
    std::ostringstream oss;
    oss << '{';
    for (auto it = column_indices_.begin(); it != column_indices_.end(); ++it) {
        if (it != column_indices_.begin()) {
            oss << ", ";
        }
        oss << *it;
    }
    oss << '}';
    LOG_INFO("Verifying UCC PAC on columns {}", oss.str());

    PreparePairs();

    auto emp_probabilities = CalculateEmpiricalProbabilities(*sorted_pairs_);
    auto [epsilon, delta] = FindEpsilonDelta(std::move(emp_probabilities));

    Vertical columns = TypedRelation().GetSchema()->GetVertical(
            util::IndicesToBitset(column_indices_, TypedRelation().GetNumColumns()));
    pac_ = model::UCCPAC{std::move(columns), epsilon, delta};

    LOG_INFO("Result: {}", pac_->ToLongString());
}

UCCPACVerifier::UCCPACVerifier() {
    DESBORDANTE_OPTION_USING;
    using namespace config;

    RegisterOption(kTableOpt(&input_table_).SetConditionalOpts({{nullptr, {kColumnIndices}}}));

    RegisterOption(IndicesOption{kColumnIndices, kDColumnIndices, nullptr}(
            &column_indices_, [this]() { return input_table_->GetNumberOfColumns(); }));
    RegisterOption(VectorMetricOption{&metric_});

    RegisterOption(Option(&min_delta_, kMaxDelta, kDMaxDelta, -1.0)
                           .SetValueCheck([](double x) { return x <= 1; })
                           .SetNormalizeFunc([](double x) -> double {
                               if (x < 0) {
                                   return -1;
                               }
                               return 1 - x;
                           }));
}

UCCPACHighlight UCCPACVerifier::GetHighlights(double eps_1, double eps_2) const {
    if (!pac_) {
        throw std::runtime_error("Execute must be called before calling GetHighlights");
    }

    if (eps_2 < 0) {
        eps_2 = pac_->GetEpsilon();
    }
    if (eps_2 <= eps_1) {
        return UCCPACHighlight{};
    }
    LOG_DEBUG("Calculating highlights from {} to {}...", eps_1, eps_2);

    auto begin = std::ranges::upper_bound(*sorted_pairs_, eps_1, {},
                                          [](TuplePair const& pair) { return pair.dist; });
    auto end = std::ranges::upper_bound(begin, sorted_pairs_->end(), eps_2, {},
                                        [](TuplePair const& pair) { return pair.dist; });

    LOG_DEBUG("Highlighted pairs [{}, {})", std::distance(sorted_pairs_->begin(), begin),
              std::distance(sorted_pairs_->begin(), end));
    return UCCPACHighlight{tuples_, tuple_type_, sorted_pairs_, std::move(begin), std::move(end)};
}
}  // namespace algos::pac_verifier
