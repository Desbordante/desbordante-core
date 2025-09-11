#include "algorithms/nd/nd_verifier/util/stats_calculator.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithms/nd/nd.h"
#include "algorithms/nd/nd_verifier/util/value_combination.h"
#include "util/logger.h"

namespace algos::nd_verifier::util {

[[nodiscard]] std::unordered_map<std::string, size_t> StatsCalculator::GetFrequencies(
        std::shared_ptr<std::vector<ValueCombination>> values,
        std::shared_ptr<std::vector<size_t>> frequencies) const {
    if (values->size() != frequencies->size()) {
        throw std::runtime_error(
                "(StatsCalulator::GetFrequencies): frequencies contains an incorrect number of "
                "values");
    }

    std::unordered_map<std::string, size_t> result;
    for (size_t i{0}; i < values->size(); ++i) {
        result.emplace((*values)[i].ToString(), (*frequencies)[i]);
    }

    return result;
}

void StatsCalculator::CalculateStats() {
    lhs_frequencies_ =
            CalculateFrequencies(lhs_values_->size(), encoded_lhs_->begin(), encoded_lhs_->end());
    rhs_frequencies_ =
            CalculateFrequencies(rhs_values_->size(), encoded_rhs_->begin(), encoded_rhs_->end());

    model::WeightType max_weight{0};
    model::WeightType min_weight{UINT_MAX};
    for (size_t code{0}; code < lhs_values_->size(); ++code) {
        size_t weight = value_deps_[code].size();

        if (weight == 0) {
            // This value is associated with no rhs values, but is somehow present in value_deps
            LOG_INFO(
                    "WARNING: Lhs value '{}''s "
                    "weight is 0, but it's present in value_deps",
                    (*lhs_values_)[code].ToString());
        }

        if (weight > max_weight) {
            max_weight = weight;
        }
        if (weight < min_weight) {
            min_weight = weight;
        }
    }

    std::vector<size_t> highlights_lhs_codes;
    for (size_t code{0}; code < lhs_values_->size(); ++code) {
        if (value_deps_[code].size() == max_weight) {
            highlights_lhs_codes.push_back(code);
        }
    }

    for (size_t highlight_lhs_code : highlights_lhs_codes) {
        std::unordered_set<size_t>& rhs_set = value_deps_[highlight_lhs_code];

        highlights_.emplace_back(lhs_values_, rhs_values_, encoded_lhs_, encoded_rhs_,
                                 lhs_frequencies_, rhs_frequencies_, highlight_lhs_code,
                                 std::move(rhs_set));
    }

    real_weight_ = max_weight;
    global_min_weight_ = min_weight;

    LOG_INFO("Minimal weight: {}", global_min_weight_);
    LOG_INFO("Maximal weight (real weight): {}", real_weight_);
}

}  // namespace algos::nd_verifier::util
