#include "algorithms/pac/pac_verifier/pac_verifier.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

#include "descriptions.h"
#include "names.h"
#include "option_using.h"
#include "tabular_data/input_table/option.h"
#include "util/logger.h"

namespace algos::pac_verifier {
void PACVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option<bool>(&distance_from_null_is_infinity_, kDistFromNullIsInfinity,
                                kDDistFromNullIsInfinity, false));

    RegisterOption(Option(&min_epsilon_, kMinEpsilon, kDMinEpsilon, kDefaultMinEpsilon));
    RegisterOption(Option(&max_epsilon_, kMaxEpsilon, kDMaxEpsilon, kDefaultMaxEpsilon));
    RegisterOption(Option(&epsilon_steps_, kEpsilonSteps, kDEpsilonSteps, kDefaultEpsilonSteps));
    RegisterOption(Option(&min_delta_, kMinDelta, kDMinDelta, kDefaultMinDelta));
    RegisterOption(Option(&diagonal_threshold_, kDiagonalThreshold, kDDiagonalThreshold,
                          kDefaultDiagonalThreshold));
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

    MakeOptionsAvailable({kMinEpsilon, kMaxEpsilon, kEpsilonSteps, kMinDelta, kDiagonalThreshold});
}

unsigned long long PACVerifier::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();
    LOG_INFO("Verifying {}...", pac_->ToLongString());

    auto epsilon_step = (max_epsilon_ - min_epsilon_) / (epsilon_steps_ - 1);
    auto satisfying_tuples = CountSatisfyingTuples(min_epsilon_, max_epsilon_, epsilon_steps_);
    std::vector<double> empirical_probabilities;
    auto const num_rows = typed_relation_->GetNumRows();
    std::ranges::transform(satisfying_tuples, std::back_inserter(empirical_probabilities),
                           [num_rows](unsigned long const number) {
                               return static_cast<double>(number) / num_rows;
                           });

    std::vector<bool> horizontal(epsilon_steps_);
    for (std::size_t i = 0; i < epsilon_steps_ - 1; ++i) {
        auto k = empirical_probabilities[i + 1] - empirical_probabilities[i];
        horizontal[i] =
                (empirical_probabilities[i + 1] >= min_delta_) && (k <= diagonal_threshold_);
    }
    horizontal[epsilon_steps_ - 1] = true;

    // Find longest sequence of 1's in horizontal
    std::size_t max_len = 0;
    std::size_t max_start = 0;
    std::size_t curr_len = 0;
    std::size_t curr_start = 0;
    for (std::size_t i = 0; i < horizontal.size(); ++i) {
        if (horizontal[i]) {
            ++curr_len;
            if (curr_len > max_len) {
                max_len = curr_len;
                max_start = curr_start;
            }
        } else {
            curr_start = i + 1;
            curr_len = 0;
        }
    }

    double delta;
    if (max_start == epsilon_steps_ - 1) {
        delta = empirical_probabilities.back();
    } else {
        delta = std::max(empirical_probabilities[max_start], min_delta_);
    }

    pac_->SetEpsilon(min_epsilon_ + max_start * epsilon_step);
    pac_->SetDelta(delta);
    LOG_INFO("Result: {}", pac_->ToLongString());
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now() - start)
                           .count();
    LOG_INFO("Validation took {}ms", elapsed);
    return elapsed;
}
}  // namespace algos::pac_verifier
