#include "core/algorithms/ar/ar_verifier/model/rule_coverage.h"

#include <algorithm>

namespace algos::ar_verifier::model {

RuleCoverage::RuleCoverage(double left_coverage, double right_coverage)
    : left_coverage(left_coverage), right_coverage(right_coverage) {}

RuleCoverage::RuleCoverage(std::vector<unsigned> const& transaction_indices,
                           std::vector<unsigned> const& left_rule_part,
                           std::vector<unsigned> const& right_rule_part)
    : left_coverage(CalculateCoverage(transaction_indices, left_rule_part)),
      right_coverage(CalculateCoverage(transaction_indices, right_rule_part)) {}

double RuleCoverage::CalculateCoverage(std::vector<unsigned> const& transaction_indices,
                                       std::vector<unsigned> const& rule_part) {
    if (transaction_indices.empty()) {
        return 0.0;
    }

    if (rule_part.empty()) {
        return 1.0;
    }

    std::vector<unsigned> intersection;
    std::ranges::set_intersection(rule_part, transaction_indices, std::back_inserter(intersection));

    return static_cast<double>(intersection.size()) / static_cast<double>(rule_part.size());
}
}  // namespace algos::ar_verifier::model
