#include "core/algorithms/pac/pac_verifier/pairwise_pac_verifier.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <utility>
#include <vector>

#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {
std::vector<std::pair<double, double>> PairWisePACVerifier::CalculateEmpiricalProbabilities(
        std::vector<TuplePair> const& sorted_pairs) const {
    // NOTE: A 10^6-row table (which is an ordinary case) will contain 10^12 pairs.
    // But if all these TuplePair objects fit in memory, std::size_t will be guaranteed to
    // be large enough to hold their number
    std::vector<std::pair<double, double>> result;

    if (sorted_pairs.empty()) {
        return {{0, 0}, {0, 1}};
    }

    // Algorithm makes some assumptions based on these properties of functions
    assert(std::abs(GetNumPairs(0)) < kDistThreshold);
    assert(std::abs(GetDelta(sorted_pairs.size()) - 1) < kDistThreshold);

    // Use ceil to ensure that min_pairs is always enough to satisfy min_delta
    std::size_t min_pairs_num = std::ceil(GetNumPairs(MinDelta()));
    assert(min_pairs_num <= sorted_pairs.size());
    std::size_t max_pairs_num = std::floor(GetNumPairs(MaxDelta()));
    assert(max_pairs_num <= sorted_pairs.size());
    LOG_TRACE("Min and max pairs: [{}, {}] (sorted pairs size: {})", min_pairs_num, max_pairs_num,
              sorted_pairs.size());

    std::size_t pairs_step;
    if (DeltaSteps() <= 1) {
        pairs_step = max_pairs_num - min_pairs_num;
    } else {
        pairs_step =
                std::round(static_cast<double>(max_pairs_num - min_pairs_num) / (DeltaSteps() - 1));
    }
    if (pairs_step == 0) {
        LOG_DEBUG("Pairs step is 0. Setting to 1");
        pairs_step = 1;
    }

    auto end = std::ranges::upper_bound(sorted_pairs, 0, {},
                                        [](TuplePair const& p) { return p.dist; });
    std::size_t curr_size = std::distance(sorted_pairs.begin(), end);
    result.emplace_back(0, GetDelta(curr_size));
    LOG_DEBUG("Delta steps: {}, pairs step: {}, initial size: {} (delta: {})", DeltaSteps(),
              pairs_step, curr_size, GetDelta(curr_size));

    if (sorted_pairs.empty()) {
        result.emplace_back(0, 1);
        return result;
    }

    auto iteration = [&](std::size_t needed_pairs_num) {
        // Find eps_i
        auto need_to_add = needed_pairs_num - curr_size;
        auto actually_add = std::min(
                need_to_add, static_cast<std::size_t>(std::distance(end, sorted_pairs.end())));
        std::advance(end, actually_add);
        curr_size += actually_add;
        // end != sorted_gamma_->begin() here, because
        //  a) we've checked that sorted_gamma_ is not empty
        //  b) we've checked that needed_pairs_num > curr_size (which is at least 0)
        assert(end != sorted_pairs.begin());
        auto eps_i = std::prev(end)->dist;
        LOG_TRACE("Eps for {} pairs: {}", needed_pairs_num, eps_i);

        // Refine delta_i
        // end is "past-the-iterator", so refining it directly will give undesired results
        std::advance(end, -1);
        RefineDelta(end);
        assert(end != sorted_pairs.end());
        std::advance(end, 1);
        curr_size = std::distance(sorted_pairs.begin(), end);
        LOG_TRACE("Refined size: {}", curr_size);
        result.emplace_back(eps_i, GetDelta(curr_size));
    };

    for (auto needed_pairs_num = min_pairs_num; needed_pairs_num < max_pairs_num;
         needed_pairs_num += pairs_step) {
        if (needed_pairs_num <= curr_size) {
            continue;
        }
        iteration(needed_pairs_num);
    }
    // Ensure that (??, max delta) and (??, 1) are always in empirical_probabilities
    iteration(max_pairs_num);
    result.emplace_back(sorted_pairs.back().dist, 1);

    return result;
}

std::pair<double, double> PairWisePACVerifier::GetEpsilonDeltaForEpsilonImpl(
        double epsilon, std::vector<TuplePair> const& pairs) const {
    auto it = std::ranges::lower_bound(pairs, epsilon, {},
                                       [](TuplePair const& pair) { return pair.dist; });
    RefineDelta(it);
    return {it->dist, GetDelta(std::distance(pairs.begin(), it))};
}
}  // namespace algos::pac_verifier
