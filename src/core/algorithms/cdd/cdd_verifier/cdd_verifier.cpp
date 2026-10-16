#include "core/algorithms/cdd/cdd_verifier/cdd_verifier.h"

#include <chrono>

#include "core/algorithms/cdd/cdd_verifier/cdd_highlight.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"

namespace algos::cdd {
CDDVerifier::CDDVerifier() : dd::DDVerifier() {
    RegisterOptions();
}

std::vector<std::size_t> CDDVerifier::GetIndicesPruningTable() {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < typed_relation_->GetNumRows(); ++i) {
        if (cdd_.IsCondsHolds(cdd_.lhs_condition_, typed_relation_, i) == std::nullopt) {
            result.emplace_back(i);
        }
    }
    return result;
}

void CDDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto const default_cdd = model::CDD();
    RegisterOption(Option{&cdd_, kCdd, kDCdd, default_cdd});
}

void CDDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kCdd, kDDudm});
}

void CDDVerifier::CheckDFOnRhs(std::vector<std::pair<std::size_t, std::size_t>> const& lhs) {
    auto check_and_add_rhs_violation = [&](std::size_t row_idx) {
        if (!row_satisfies_rhs_[row_idx] && !added_to_cdd_highlights_[row_idx]) {
            if (auto failed_idx =
                        model::CDD::IsCondsHolds(cdd_.rhs_condition_, typed_relation_, row_idx)) {
                cdd_highlights_.emplace_back(row_idx, *failed_idx);
                added_to_cdd_highlights_[row_idx] = true;
            }
        }
    };

    for (auto const& pair : lhs) {
        check_and_add_rhs_violation(pair.first);
        check_and_add_rhs_violation(pair.second);

        auto curr_constraint = dd_.right.cbegin();
        bool is_error = false;
        for (auto const column_index : rhs_column_indices_) {
            if (double const dif = CalculateDistance(column_index, pair);
                !curr_constraint->constraint.Contains(dif)) {
                highlights_.emplace_back(column_index, pair, dif);
                is_error = true;
            }
            ++curr_constraint;
        }
        if (is_error) {
            ++num_error_rhs_;
        }
    }
}

void CDDVerifier::ExecuteInternal() {
    ids_ = GetIndicesPruningTable();
    if (!ids_.empty()) {
        dd_ = cdd_.dd_;

        row_satisfies_rhs_.assign(typed_relation_->GetNumRows(), false);
        added_to_cdd_highlights_.assign(typed_relation_->GetNumRows(), false);
        for (std::size_t i = 0; i < typed_relation_->GetNumRows(); ++i) {
            row_satisfies_rhs_[i] = (model::CDD::IsCondsHolds(cdd_.rhs_condition_, typed_relation_,
                                                              i) == std::nullopt);
        }

        VerifyDD();
    } else {
        throw std::runtime_error("No rows in the table satisfy the CDD conditions");
    }
}

bool CDDVerifier::DDHolds() const {
    return !num_error_rhs_ && !cdd_highlights_.size();
}

}  // namespace algos::cdd
