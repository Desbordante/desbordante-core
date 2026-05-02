#include "core/algorithms/pac/pac_verifier/pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {
void PACVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option(&min_epsilon_, kMinEpsilon, kDMinEpsilon, -1.0));
    RegisterOption(Option(&max_epsilon_, kMaxEpsilon, kDMaxEpsilon, -1.0));
    RegisterOption(Option(&delta_steps_, kDeltaSteps, kDDeltaSteps, 0ul));
    RegisterOption(Option(&diagonal_threshold_, kDiagonalThreshold, kDDiagonalThreshold,
                          kDefaultDiagonalThreshold));
}

void PACVerifier::ProcessCommonExecuteOpts() {
    // "Auto" values for min delta and max delta
    if (MinDelta() < 0) {
        if (min_epsilon_ >= 0 || max_epsilon_ >= 0) {
            SetMinDelta(0);
        } else {
            SetMinDelta(kDefaultMinDelta);
        }
    }
    if (MaxDelta() < 0) {
        if (min_epsilon_ >= 0 || max_epsilon_ >= 0) {
            SetMaxDelta(1);
        } else {
            SetMaxDelta(kDefaultMaxDelta);
        }
    }

    if (MaxDelta() < MinDelta()) {
        throw config::ConfigurationError("Max delta must be greater or equal to min delta");
    }

    if (delta_steps_ == 0) {
        delta_steps_ = std::round((MaxDelta() - MinDelta()) * 1000);
    }

    // Ignore delta bounds if min_eps == max_eps ("validation")
    if (min_epsilon_ >= 0 && std::abs(min_epsilon_ - max_epsilon_) < kDistThreshold) {
        SetMinDelta(0);
        SetMaxDelta(1);
    }

    if (max_epsilon_ > 0 && max_epsilon_ < min_epsilon_) {
        throw config::ConfigurationError("Min epsilon must be less or equal to max epsilon");
    }

    LOG_DEBUG("Common PAC verifier options: epsilons: [{}, {}], deltas: [{}, {}], delta steps: {}",
              min_epsilon_, max_epsilon_, MinDelta(), MaxDelta(), delta_steps_);
}

unsigned long long PACVerifier::ExecuteInternal() {
    ProcessCommonExecuteOpts();

    auto start = std::chrono::system_clock::now();
    PACTypeExecuteInternal();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now() - start)
                           .count();
    LOG_INFO("Validation took {}ms", elapsed);
    return elapsed;
}

void PACVerifier::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true);
    input_table_->Reset();
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: PAC validation is meaningless.");
    }

    ProcessPACTypeOptions();
    PreparePACTypeData();
}

void PACVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kMinEpsilon, kMaxEpsilon, kDeltaSteps, kDiagonalThreshold});
}

std::pair<double, double> PACVerifier::FindEpsilonDelta(
        std::vector<std::pair<double, double>>&& empirical_probabilities) const {
    // At least two pairs must always be in empirical_probabilities: (0, ??) and (1, ??)
    assert(empirical_probabilities.size() >= 2);

    LOG_TRACE("Empirical probabilities:");
    for ([[maybe_unused]] auto const& [eps, delta] : empirical_probabilities) {
        LOG_TRACE("\t{}, {}", eps, delta);
    }

    auto begin = empirical_probabilities.begin();
    auto end = empirical_probabilities.end();

    // First pair is always (0, ??), others have their delta >= min delta...
    if (begin->first < kDistThreshold && std::next(begin)->second > MinDelta() - kDistThreshold) {
        // ...but there is a corner case: ?? >= min_delta
        if (begin->second < MinDelta() - kDistThreshold) {
            std::advance(begin, 1);
        }
    } else {
        // Be careful: ??) is a trigraph for ]
        LOG_WARN(
                "First two empirical probability pairs must be (0, xx) and (xx, {}), got ({}, {}) "
                "and ({}, {})",
                MinDelta(), begin->first, begin->second, std::next(begin)->first,
                std::next(begin)->second);
        begin = std::ranges::lower_bound(begin, end, MinDelta(), {},
                                         [](auto const& p) { return p.second; });
        // Empirical probabilities must always contain (??, 1)
        assert(begin != end);
    }
    // And the last pair is always (??, 1), others have their delta <= max_delta
    auto const& back = *std::prev(end);
    auto const& pre_back = *std::prev(end, 2);
    if (std::abs(back.second - 1) < kDistThreshold &&
        pre_back.second < MaxDelta() + kDistThreshold) {
        // Again, we have a corner case: max delta >= 1
        if (MaxDelta() < 1) {
            std::advance(end, -1);
        }
    } else {
        LOG_WARN(
                "Last two empirical probability pairs must be (xx, {}) and (xx, 1), got ({}, "
                "{}) "
                "and ({}, {})",
                MaxDelta(), pre_back.first, pre_back.second, back.first, back.second);
        end = std::ranges::upper_bound(begin, end, MaxDelta(), {},
                                       [](auto const& p) { return p.second; });
    }

    if (max_epsilon_ >= 0) {
        // Special case: max_eps and min_delta cannot be both satisfied.
        // Return (??, min_delta) so that user can see that parameters are contradictory.
        // NOTE: This should be checked before min_epsilon, because (??, min_epsilon) will
        // always have its first <= max_epsilon
        if (begin->first > max_epsilon_) {
            LOG_DEBUG(
                    "Max eps and min delta cannot be both satisfied. Taking pair with min "
                    "delta.");
            return *begin;
        }

        // Take all values that have eps < max_eps
        // New pair is not added, because it would be stripped out anyway
        end = std::ranges::upper_bound(
                begin, end, max_epsilon_, {},
                [](std::pair<double, double> const& pair) { return pair.first; });
        assert(begin != end);
    }
    if (min_epsilon_ >= 0) {
        // Take all values that have eps > min_eps, and add (min_eps, delta_{j - 1}) to
        // beginning (where j is the index of the first "good" element)
        begin = std::ranges::upper_bound(
                begin, end, min_epsilon_, {},
                [](std::pair<double, double> const& pair) { return pair.first; });
        // Don't add (min_eps, delta_{j - 1}) if j == 0, because delta_{-1} is less than
        // min_delta
        if (begin != empirical_probabilities.begin()) {
            std::advance(begin, -1);
            *begin = GetEpsilonDeltaForEpsilon(min_epsilon_);
        }

        // If begin != empirical_probs.begin, a new pair is added.
        // If begin == empirical_probs.begin, then begin != empirical_probs.end (because
        // empirical_probs is not empty here)
        assert(begin != end);
    }

    // Due to the delta refinement, there may be several epsilons with near deltas
    auto extra_pairs = std::ranges::unique(
            begin, end,
            [threshold{diagonal_threshold_}](double a, double b) { return b - a < threshold; },
            [](std::pair<double, double> const& pair) { return pair.second; });
    // For convenience only
    auto stripped_emp_prob = std::ranges::subrange(begin, extra_pairs.begin());

    if (stripped_emp_prob.size() == 1) {
        return stripped_emp_prob.front();
    }

    LOG_DEBUG("Stripped empirical probabilities:");
    for ([[maybe_unused]] auto const& [eps, delta] : stripped_emp_prob) {
        LOG_DEBUG("\t{}, {}", eps, delta);
    }

    double max_eps_diff = -1;
    std::size_t best_eps_idx = 0;
    for (std::size_t i = 0; i < stripped_emp_prob.size() - 1; ++i) {
        auto eps_diff = stripped_emp_prob[i + 1].first - stripped_emp_prob[i].first;
        // Take the least eps_i such that (eps_{i + 1} - eps_i) is maximal
        if (eps_diff > max_eps_diff) {
            max_eps_diff = eps_diff;
            best_eps_idx = i;
        }
    }

    if (max_eps_diff < 0) {
        return stripped_emp_prob.back();
    }

    return stripped_emp_prob[best_eps_idx];
}
}  // namespace algos::pac_verifier
