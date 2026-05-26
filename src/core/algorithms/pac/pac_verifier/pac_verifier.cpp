#include "core/algorithms/pac/pac_verifier/pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/util/logger.h"

namespace {
double Epsilon(std::pair<double, double> const& epsilon_delta) {
    return epsilon_delta.first;
}

double Delta(std::pair<double, double> const& epsilon_delta) {
    return epsilon_delta.second;
}
}  // namespace

namespace algos::pac_verifier {
void PACVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option(&min_epsilon_, kMinEpsilon, kDMinEpsilon, -1.0));
    RegisterOption(Option(&max_epsilon_, kMaxEpsilon, kDMaxEpsilon, -1.0));
    RegisterOption(Option(&min_delta_, kMinDelta, kDMinDelta, -1.0).SetValueCheck([](double x) {
        return x <= 1;
    }));
    RegisterOption(Option(&delta_steps_, kDeltaSteps, kDDeltaSteps, 0ul));
    RegisterOption(Option(&diagonal_threshold_, kDiagonalThreshold, kDDiagonalThreshold,
                          kDefaultDiagonalThreshold));
}

void PACVerifier::ProcessCommonExecuteOpts() {
    if (min_delta_ < 0) {
        if (min_epsilon_ >= 0 || max_epsilon_ >= 0) {
            min_delta_ = 0;
        } else {
            min_delta_ = kDefaultMinDelta;
        }
    }

    if (delta_steps_ == 0) {
        delta_steps_ = std::round((1 - min_delta_) * 1000);
    }

    if (max_epsilon_ > 0 && max_epsilon_ < min_epsilon_) {
        throw config::ConfigurationError("Min epsilon must be less or equal to max epsilon");
    }

    LOG_DEBUG(
            "Common PAC verifier options: min delta: {}, min eps: {}, max eps: {}, delta steps: {}",
            min_delta_, min_epsilon_, max_epsilon_, delta_steps_);
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

std::optional<PACVerifier::EpsilonDelta> PACVerifier::TryValidatePAC(
        std::vector<EpsilonDelta> const& empirical_probabilities) const {
    if (max_epsilon_ >= 0 && min_epsilon_ >= 0 && max_epsilon_ - min_epsilon_ < kDistThreshold) {
        LOG_DEBUG("Got min_epsilon == max_epsilon, entering epsilon validation mode");
        return GetEpsilonDeltaForEpsilon(min_epsilon_);
    }
    if (max_epsilon_ >= 0 && min_delta_ >= 0) {
        // Both epsilon and delta bounds are passed explicitly. It may be "delta validation"
        auto first_delta_it =
                std::ranges::lower_bound(empirical_probabilities, min_delta_, {}, Delta);
        if (Epsilon(*first_delta_it) > max_epsilon_) {
            LOG_DEBUG(
                    "Max eps and min delta cannot be both satisfied. Taking first pair with min "
                    "delta.");
            return *first_delta_it;
        }
    }
    return std::nullopt;
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

    MakeOptionsAvailable({kMinEpsilon, kMaxEpsilon, kMinDelta, kDeltaSteps, kDiagonalThreshold});
}

PACVerifier::EpsilonDelta PACVerifier::FindEpsilonDelta(
        std::vector<EpsilonDelta>&& empirical_probabilities) const {
    assert(!empirical_probabilities.empty());

    LOG_TRACE("Empirical probabilities:");
    for ([[maybe_unused]] auto const& [eps, delta] : empirical_probabilities) {
        LOG_TRACE("\t{}, {}", eps, delta);
    }

    auto maybe_eps_delta = TryValidatePAC(empirical_probabilities);
    if (maybe_eps_delta) {
        return *maybe_eps_delta;
    }

    if (empirical_probabilities.size() == 1) {
        return empirical_probabilities.front();
    }

    auto begin = empirical_probabilities.begin();
    auto end = empirical_probabilities.end();

    // First pair is always (0, ??), others have their delta >= min_delta...
    if (Epsilon(*begin) < kDistThreshold &&
        Delta(*std::next(begin)) > min_delta_ - kDistThreshold) {
        // ...but there is a corner case: ?? >= min_delta
        if (Delta(*begin) < min_delta_ - kDistThreshold) {
            std::advance(begin, 1);
        }
    } else {
        // Be careful: ??) is a trigraph for ]
        LOG_WARN(
                "First two empirical probability pairs must be (0, xx) and (xx, {}), got ({}, {}) "
                "and ({}, {})",
                min_delta_, Epsilon(*begin), Delta(*begin), Epsilon(*std::next(begin)),
                Epsilon(*std::next(begin)));
        begin = std::ranges::lower_bound(begin, end, min_delta_, {}, Delta);
        // Empirical probabilities must always contain (??, 1)
        assert(begin != end);
    }

    if (max_epsilon_ >= 0) {
        // Already checked earlier
        assert(Epsilon(*begin) <= max_epsilon_);

        // Take all values that have eps < max_eps
        // New pair is not added, because it would be stripped out anyway
        end = std::ranges::upper_bound(begin, end, max_epsilon_, {}, Epsilon);
        assert(begin != end);
    }
    if (min_epsilon_ >= 0) {
        // Take all values that have eps > min_eps, and add (min_eps, delta_{j - 1}) to beginning
        // (where j is the index of the first "good" element)
        begin = std::ranges::upper_bound(begin, end, min_epsilon_, {}, Epsilon);
        auto eps_delta_pair = GetEpsilonDeltaForEpsilon(min_epsilon_);
        if (max_epsilon_ > 0 && Epsilon(eps_delta_pair) > max_epsilon_) {
            LOG_DEBUG("No pairs between min eps and max eps. Taking a pair before min eps");
            if (begin == empirical_probabilities.begin()) {
                // No pair before min_eps. It is only possible in "Warning" case -- if concrete
                // algorithm has violated the contract
                LOG_WARN("All pairs have their epsilon > min epsilon");
                return {0, 0};
            }
            // Take eps < min_eps, refine it, and then clamp to max_epsilon (we already know that
            // refine(min_eps) > max_eps, so no explicit refinement needed)
            // TODO: This needs more thought with reversed refinement direction
            // (RefinementDirection()? -- should be avoided at all costs)
            auto eps = Epsilon(*std::prev(begin));
            auto [epsilon, delta] = GetEpsilonDeltaForEpsilon(eps);
            return {std::min(epsilon, max_epsilon_), delta};
        }
        if (Delta(eps_delta_pair) > min_delta_ - kDistThreshold || begin == end) {
            std::advance(begin, -1);
            *begin = eps_delta_pair;
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
            Delta);
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
        auto eps_diff = Epsilon(stripped_emp_prob[i + 1]) - Epsilon(stripped_emp_prob[i]);
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
