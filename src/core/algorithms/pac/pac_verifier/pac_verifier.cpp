#include "core/algorithms/pac/pac_verifier/pac_verifier.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {
void PACVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option<bool>(&distance_from_null_is_infinity_, kDistFromNullIsInfinity,
                                kDDistFromNullIsInfinity, false));

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
        delta_steps_ = (1 - min_delta_) * 1000;
    }

    // Ignore min_delta if min_eps == max_eps ("validation")
    if (min_epsilon_ >= 0 && min_epsilon_ == max_epsilon_) {
        min_delta_ = 0;
    }

    if (max_epsilon_ > 0 && max_epsilon_ < min_epsilon_) {
        throw config::ConfigurationError("Min epsilon must be less or equal to max epsilon");
    }
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

    MakeOptionsAvailable({kMinEpsilon, kMaxEpsilon, kMinDelta, kDeltaSteps, kDiagonalThreshold});
}

std::pair<double, double> PACVerifier::FindEpsilonDelta(
        std::vector<std::pair<double, double>>&& empirical_probabilities) const {
    if (empirical_probabilities.empty()) {
        return {0, 0};
    }

    LOG_TRACE("Empirical probabilities:");
    for ([[maybe_unused]] auto const& [eps, delta] : empirical_probabilities) {
        LOG_TRACE("\t{}, {}", eps, delta);
    }

    auto begin = empirical_probabilities.begin();
    auto end = empirical_probabilities.end();

    // Min delta is checked to be less or equal than 1, and empirical probabilies always contain
    // (??, 1), so begin cannot be equal to empirical_probabilities.end() here
    begin = std::ranges::lower_bound(begin, end, min_delta_, {},
                                     [](auto const& p) { return p.second; });

    if (max_epsilon_ >= 0) {
        // Special case: max_eps and min_delta cannot be both satisfied.
        // Return (??, min_delta) so that user can see that parameters are contradictory.
        if (begin->first > max_epsilon_) {
            LOG_TRACE(
                    "Max eps and min delta cannot be both satisfied. Taking pair with min delta.");
            return empirical_probabilities[1];
        }

        // Take all values that have eps < max_eps
        // New pair is not added, because it would be stripped out anyway
        end = std::ranges::upper_bound(
                begin, end, max_epsilon_, {},
                [](std::pair<double, double> const& pair) { return pair.first; });
    }
    if (min_epsilon_ >= 0) {
        // Take all values that have eps > min_eps, and add (min_eps, delta_{j - 1}) to beginning
        // (where j is the index of the first "good" element)
        begin = std::ranges::upper_bound(
                begin, end, min_epsilon_, {},
                [](std::pair<double, double> const& pair) { return pair.first; });
        // Don't add (min_eps, delta_{j - 1}) if j == 0, because delta_{-1} is less than min_delta
        if (begin != empirical_probabilities.begin()) {
            std::advance(begin, -1);
            begin->first = min_epsilon_;
        }
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
    if (stripped_emp_prob.size() == 2) {
        // We have two pairs: (0, delta_1) and (eps_2, 1)
        // We don't have enough information to decide which pair is better, so let's use simple
        // heuristic based on min_delta
        if (stripped_emp_prob.front().second >= min_delta_) {
            return stripped_emp_prob.front();
        }
        return stripped_emp_prob.back();
    }

    LOG_TRACE("Stripped empirical probabilities:");
    for ([[maybe_unused]] auto const& [eps, delta] : stripped_emp_prob) {
        LOG_TRACE("\t{}, {}", eps, delta);
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
