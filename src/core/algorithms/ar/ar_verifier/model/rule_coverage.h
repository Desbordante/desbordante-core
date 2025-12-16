#pragma once

#include <vector>

namespace algos::ar_verifier::model {
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

    RuleCoverage(double left_coverage, double right_coverage);

    RuleCoverage(std::vector<unsigned> const& transaction_indices,
                 std::vector<unsigned> const& left_rule_part,
                 std::vector<unsigned> const& right_rule_part);

private:
    static double CalculateCoverage(std::vector<unsigned> const& transaction_indices,
                                    std::vector<unsigned> const& rule_part);
};
}  // namespace algos::ar_verifier::model
