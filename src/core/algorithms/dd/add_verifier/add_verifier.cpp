#include "core/algorithms/dd/add_verifier/add_verifier.h"

#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos::dd {

ADDVerifier::ADDVerifier() : DDVerifier() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void ADDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto check_threshold = [](double parameter) {
        if (parameter < 0 || parameter > 1)
            throw config::ConfigurationError("Satisfaction threshold out of range");
    };

    RegisterOption(
            Option{&satisfaction_threshold_, kSatisfactionThreshold, kDSatisfactionThreshold, 0.}
                    .SetValueCheck(check_threshold));
}

void ADDVerifier::CheckDFOnRhs(std::vector<std::pair<std::size_t, std::size_t>> const &lhs) {
    double min_dist = -1.;
    for (auto const &pair : lhs) {
        auto curr_constraint = dd_.right.cbegin();
        for (auto const column_index : rhs_column_indices_) {
            double const dif = CalculateDistance(column_index, pair);
            if (!curr_constraint->constraint.Contains(dif)) {
                highlights_.emplace_back(column_index, pair, dif);
            	++num_error_rhs_;
	    }
            min_dist = min_dist == -1 ? dif : std::min(min_dist, dif);
            ++curr_constraint;
        }
    }
    std::size_t num_pairs_with_min_dist = 0;
    for (auto const &pair : lhs) {
        auto curr_constraint = dd_.right.cbegin();
        for (auto const column_index : rhs_column_indices_) {
            double const dif = CalculateDistance(column_index, pair);
            if (dif == min_dist) {
                ++num_pairs_with_min_dist;
            }
            ++curr_constraint;
        }
    }
    if (min_dist < dd_.right.cbegin()->constraint.lower_bound || min_dist > dd_.right.cbegin()->constraint.upper_bound) {
        error_ = 1.;
    } else {
        error_ = 1. - num_pairs_with_min_dist / lhs.size();
    }
}

bool ADDVerifier::DDHolds() const {
    return 1 - error_ >= satisfaction_threshold_;
}

void ADDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kDDString});
    MakeOptionsAvailable({kDDString, kSatisfactionThreshold});
}

void DDVerifier::CheckCorrectnessDd() const {
    auto check_constraint = [](auto const &constraint) {
        if (constraint.constraint.upper_bound < constraint.constraint.lower_bound) {
            throw std::invalid_argument("Invalid constraint bounds for column: " +
                                        constraint.column_name);
        }
        if (constraint.constraint.lower_bound < 0 || constraint.constraint.upper_bound < 0) {
            throw std::invalid_argument("Constraint bounds cannot be negative for column: " +
                                        constraint.column_name);
        }
    };

    std::ranges::for_each(dd_.left, check_constraint);
    std::ranges::for_each(dd_.right, check_constraint);
    if (dd_.right.size() > 1) {
        throw std::invalid_argument(
                "The RHS of an ADD must have a differential function on a single attribute.");
    }
}

}  // namespace algos::dd
