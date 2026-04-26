#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
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
            sorted_pairs_->emplace_back(i, j, metric_.Dist(first, second));
        }
    }
    std::ranges::sort(*sorted_pairs_, {}, [](TuplePair const& p) { return p.dist; });
}

std::vector<std::pair<double, double>> UCCPACVerifier::CalculateEmpiricalProbabilities() const {
    std::size_t total_tuples = tuples_->size();
    std::size_t total_pairs = total_tuples * total_tuples;

    std::vector<std::pair<double, double>> result;

    // Unlike FD PAC verifier, sorted pairs cannot be empty
    std::size_t min_pairs_num = std::ceil(GetNumPairs(MinDelta()));
    LOG_TRACE("Min pairs num: {}", min_pairs_num);

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

    auto end = std::ranges::upper_bound(*sorted_pairs_, 0, {},
                                        [](TuplePair const& p) { return p.dist; });
    std::size_t curr_size = std::distance(sorted_pairs_->begin(), end);
    result.emplace_back(0, GetDelta(curr_size));
    LOG_DEBUG("Delta steps: {}, pairs step: {}, initial size: {} (delta: {})", DeltaSteps(),
              pairs_step, curr_size, GetDelta(curr_size));

    auto iteration = [&](std::size_t needed_pairs_num) {
        // Find eps_i
        auto need_to_add = needed_pairs_num - curr_size;
        auto actually_add = std::min(
                need_to_add, static_cast<std::size_t>(std::distance(end, sorted_pairs_->end())));
        std::advance(end, actually_add);
        curr_size += actually_add;
        // end != sorted_pairs_->begin() here, because
        //  a) sorted_pairs_ is not empty
        //  b) we've checked that needed_pairs_num > curr_size (which is at least 0)
        assert(end != sorted_pairs_->begin());
        auto eps_i = std::prev(end)->dist;
        LOG_TRACE("Eps for {} pairs: {}", needed_pairs_num, eps_i);

        // Refine delta_i
        while (end != sorted_pairs_->end() && end->dist - eps_i < kDistThreshold) {
            std::advance(end, 1);
            ++curr_size;
        }
        assert(curr_size == static_cast<std::size_t>(std::distance(sorted_pairs_->begin(), end)));
        LOG_TRACE("Refined size: {}", curr_size);
        result.emplace_back(eps_i, GetDelta(curr_size));
    };
    for (auto needed_pairs_num = min_pairs_num; needed_pairs_num < sorted_pairs_->size();
         needed_pairs_num += pairs_step) {
        if (needed_pairs_num <= curr_size) {
            continue;
        }
        iteration(needed_pairs_num);
    }
    // Ensure that (??, 1) is always in empirical_probabilities
    iteration(sorted_pairs_->size());

    return result;
}

UCCPACVerifier::UCCPACVerifier() : PACVerifier() {
    DESBORDANTE_OPTION_USING;
    using namespace config;

    RegisterOption(
            kTableOpt(&input_table_).SetConditionalOpts({{nullptr, {kColumnIndices, kMetric}}}));

    RegisterOption(IndicesOption{kColumnIndices, kDColumnIndices, nullptr}(
            &column_indices_, [this]() { return input_table_->GetNumberOfColumns(); }));
    RegisterOption()
}
}  // namespace algos::pac_verifier
