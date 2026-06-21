#include "core/algorithms/pac/pac_verifier/pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace algos::pac_verifier {
void PACVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(
            Option(&min_epsilon_, kMinEpsilon, kDMinEpsilon, 0.0).SetValueCheck([](double x) {
                return x >= 0;
            }));
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

    if (max_epsilon_ > 0 && max_epsilon_ < min_epsilon_) {
        throw config::ConfigurationError("Min epsilon must be less or equal to max epsilon");
    }

    LOG_DEBUG("Common PAC verifier options: epsilons: [{}, {}], deltas: [{}, {}], delta steps: {}",
              min_epsilon_, max_epsilon_, MinDelta(), MaxDelta(), delta_steps_);
}

unsigned long long PACVerifier::ExecuteInternal() {
    ProcessCommonExecuteOpts();

    auto elapsed = util::TimedInvoke([this]() { PACTypeExecuteInternal(); });
    LOG_INFO("Validation took {}ms", elapsed);
    return elapsed;
}

std::optional<PACVerifier::EpsilonDelta> PACVerifier::TryValidatePAC(
        std::vector<EpsilonDelta> const& empirical_probabilities) const {
    // It's convenient to call delta validation like min_eps = max_eps = 0, delta = delta,
    // so these checks should be first
    if (max_epsilon_ >= 0) {
        // Both epsilon and delta bounds are passed explicitly. It may be "delta validation"
        auto first_delta_it =
                std::ranges::lower_bound(empirical_probabilities, MinDelta(), {}, GetDelta);
        if (first_delta_it->epsilon > max_epsilon_) {
            LOG_DEBUG(
                    "Max eps and min delta cannot be both satisfied. Taking first pair with min "
                    "delta.");
            return *first_delta_it;
        }
    }
    if (min_epsilon_ >= 0 && MaxDelta() < 1) {
        auto after_delta_it =
                std::ranges::upper_bound(empirical_probabilities, MaxDelta(), {}, GetDelta);
        auto last_delta_it = std::prev(after_delta_it);
        if (last_delta_it->epsilon < min_epsilon_) {
            LOG_DEBUG(
                    "Min eps and max delta cannot be both satisfied. Taking last pair with max "
                    "delta.");
            return *last_delta_it;
        }
    }
    if (max_epsilon_ >= 0 && min_epsilon_ >= 0 && max_epsilon_ - min_epsilon_ < kDistThreshold) {
        LOG_DEBUG("Got min_epsilon == max_epsilon, entering epsilon validation mode");
        return GetEpsilonDeltaForEpsilon(min_epsilon_);
    }
    return std::nullopt;
}

std::optional<PACVerifier::EpsilonDelta> PACVerifier::CheckPairsBetweenMinMaxEpsilon(
        std::vector<EpsilonDelta> const& empirical_probabilities) const {
    assert(empirical_probabilities.size() >= 2);

    if (max_epsilon_ >= 0 && min_epsilon_ > 0) {
        auto after_min_epsilon =
                std::ranges::upper_bound(empirical_probabilities, min_epsilon_, {}, GetEpsilon);
        if (after_min_epsilon->epsilon > max_epsilon_) {
            LOG_INFO("No pairs between min eps and max eps. Taking a pair after max eps");
            auto eps_delta_pair = GetEpsilonDeltaForEpsilon(min_epsilon_);
            // Either `GetEpsilonDeltaForEpsilon` will produce a valid pair with eps <= max_eps,
            // or it will take the least epsilon possible
            return EpsilonDelta{std::max(min_epsilon_, eps_delta_pair.epsilon),
                                eps_delta_pair.delta};
        }
    }
    if (min_epsilon_ > 0) {
        auto eps_delta_pair = GetEpsilonDeltaForEpsilon(min_epsilon_);
        if (eps_delta_pair.epsilon > empirical_probabilities.back().epsilon - kDistThreshold) {
            // Case when max delta < 1 must have been checked in `TryValidatePAC`
            assert(MaxDelta() > 1 - kDistThreshold);
            LOG_INFO("No pairs after min eps. Taking last pair");
            // Let `GetEpsilonDeltaForEpsilon` select delta here
            return eps_delta_pair;
        }
    }
    if (max_epsilon_ >= 0) {
        auto after_max_epsilon =
                std::ranges::upper_bound(empirical_probabilities, max_epsilon_, {}, GetEpsilon);
        // By "first pairs invariant", first epsilon must be 0
        assert(after_max_epsilon != empirical_probabilities.begin());
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

    MakeOptionsAvailable({kMinEpsilon, kMaxEpsilon, kDeltaSteps, kDiagonalThreshold});
}

std::ranges::subrange<std::vector<PACVerifier::EpsilonDelta>::const_iterator>
PACVerifier::BuildECDF(std::vector<EpsilonDelta>& empirical_probabilities) const {
    auto begin = empirical_probabilities.begin();
    auto end = empirical_probabilities.end();

    // First pair is always (0, ??), others have their delta >= min delta...
    if (begin->epsilon < kDistThreshold && std::next(begin)->delta > MinDelta() - kDistThreshold) {
        // ...but there is a corner case: ?? >= min_delta
        if (begin->delta < MinDelta() - kDistThreshold) {
            std::advance(begin, 1);
        }
    } else {
        // Be careful: ??) is a trigraph for ]
        LOG_ERROR(
                "First two empirical probability pairs must be (0, xx) and (xx, {}), got ({}, {}) "
                "and ({}, {})",
                MinDelta(), begin->epsilon, begin->delta, std::next(begin)->epsilon,
                std::next(begin)->delta);
        assert(false);
    }
    // And the last pair is always (??, 1), others have their delta <= max_delta
    auto const& back = *std::prev(end);
    auto const& pre_back = *std::prev(end, 2);
    if (std::abs(back.delta) - 1 < kDistThreshold && pre_back.delta < MaxDelta() + kDistThreshold) {
        // There is a corner case: max delta == 1
        if (MaxDelta() < 1) {
            std::advance(end, -1);
        }
    } else {
        LOG_ERROR(
                "Last two empirical probability pairs must be (xx, {}) and (xx, 1), got ({}, {}) "
                "and ({}, {})",
                MaxDelta(), pre_back.epsilon, pre_back.delta, back.epsilon, back.delta);
        assert(false);
    }

    if (min_epsilon_ > 0) {
        // Take all values that have eps > min_eps, and add (min_eps, delta_{j - 1}) to beginning
        // (where j is the index of the first "good" element)
        begin = std::ranges::upper_bound(begin, end, min_epsilon_, {}, GetEpsilon);
        auto eps_delta_pair = GetEpsilonDeltaForEpsilon(min_epsilon_);
        // We've already checked that there are pairs between min_epsilon and max_epsilon
        if (max_epsilon_ >= 0) {
            assert(eps_delta_pair.epsilon <= max_epsilon_);
        }
        if (eps_delta_pair.delta > MinDelta() - kDistThreshold || begin == end) {
            LOG_DEBUG("Adding ({}, {}) to the beginning", eps_delta_pair.epsilon,
                      eps_delta_pair.delta);
            std::advance(begin, -1);
            auto epsilon = std::max(min_epsilon_, eps_delta_pair.epsilon);
            *begin = {epsilon, eps_delta_pair.delta};
        }

        // If begin != empirical_probs.begin, a new pair is added.
        // If begin == empirical_probs.begin, then begin != empirical_probs.end (because
        // empirical_probs is not empty here)
        assert(begin != end);
    }
    if (max_epsilon_ >= 0) {
        // We've already checked that there are pairs between min_eps and max_eps
        assert(begin->epsilon <= max_epsilon_);

        // Take all values that have eps < max_eps
        // New pair is not added, because it would be stripped out anyway
        end = std::ranges::upper_bound(begin, end, max_epsilon_, {}, GetEpsilon);
        // Since `begin->epsilon <= max_epsilon`, upper_bound cannot give `begin`
        assert(begin != end);
    }

    // Due to the delta refinement, there may be several epsilons with near deltas
    auto extra_pairs = std::ranges::unique(
            begin, end,
            [threshold{diagonal_threshold_}](double a, double b) { return b - a < threshold; },
            GetDelta);
    return std::ranges::subrange(begin, extra_pairs.begin());
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
        LOG_TRACE("Got epsilon-delta from TryValidatePAC");
        return *maybe_eps_delta;
    }
    maybe_eps_delta = CheckPairsBetweenMinMaxEpsilon(empirical_probabilities);
    if (maybe_eps_delta) {
        LOG_TRACE("Got epsilon-delta from CheckPairsBetweenMinMaxEpsilon");
        return *maybe_eps_delta;
    }

    auto stripped_emp_prob = BuildECDF(empirical_probabilities);

    assert(!stripped_emp_prob.empty());
    if (stripped_emp_prob.size() == 1) {
        LOG_DEBUG("Empirical probabilities contains a single pair");
        return stripped_emp_prob.front();
    }

    LOG_DEBUG("Stripped empirical probabilities:");
    for ([[maybe_unused]] auto const& [eps, delta] : stripped_emp_prob) {
        LOG_DEBUG("\t{}, {}", eps, delta);
    }

    double max_eps_diff = -1;
    std::size_t best_eps_idx = 0;
    for (std::size_t i = 0; i < stripped_emp_prob.size() - 1; ++i) {
        auto eps_diff = stripped_emp_prob[i + 1].epsilon - stripped_emp_prob[i].epsilon;
        // Take the least eps_i such that (eps_{i + 1} - eps_i) is maximal
        if (eps_diff > max_eps_diff - kDistThreshold) {
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
