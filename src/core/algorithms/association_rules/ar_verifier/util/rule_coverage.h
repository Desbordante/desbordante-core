#pragma once

#include <algorithm>
#include <set>
#include <vector>

namespace algos::ar_verifier::util {

struct RuleCoverage {
    double left_coverage = 0.0;
    double right_coverage = 0.0;

    [[nodiscard]] bool IsRuleFull() const {
        return left_coverage == 1.0 && right_coverage == 1.0;
    }

    [[nodiscard]] bool IsLeftFull() const {
        return left_coverage == 1.0;
    }

    [[nodiscard]] bool IsRightFull() const {
        return right_coverage == 1.0;
    }

    [[nodiscard]] bool IsLeftPresented() const {
        return left_coverage != 0.0;
    }

    [[nodiscard]] bool IsRightPresented() const {
        return right_coverage != 0.0;
    }

    RuleCoverage() = default;

    RuleCoverage(double const left_coverage, double const right_coverage)
        : left_coverage(left_coverage), right_coverage(right_coverage) {}

    RuleCoverage(std::vector<unsigned> const& transaction_indices,
                 std::vector<unsigned> const& left_rule_part,
                 std::vector<unsigned> const& right_rule_part)
        : left_coverage(CalculateCoverage(transaction_indices, left_rule_part)),
          right_coverage(CalculateCoverage(transaction_indices, right_rule_part)) {}

private:
    static double CalculateCoverage(std::vector<unsigned> const& transaction_indices,
                                    std::vector<unsigned> const& rule_part) {
        if (transaction_indices.empty()) {
            return 0.0;
        }

        if (rule_part.empty()) {
            return 1.0;
        }

        std::set transaction_set(transaction_indices.begin(), transaction_indices.end());
        std::set rule_set(rule_part.begin(), rule_part.end());

        std::vector<unsigned> intersection;
        std::ranges::set_intersection(rule_set, transaction_set, std::back_inserter(intersection));

        return static_cast<double>(intersection.size()) / static_cast<double>(rule_set.size());
    }
};
}  // namespace algos::ar_verifier::util
