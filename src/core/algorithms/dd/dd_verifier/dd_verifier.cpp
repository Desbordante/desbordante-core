#include "core/algorithms/dd/dd_verifier/dd_verifier.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/vertical.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace algos::dd {
DDVerifier::DDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void DDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto const default_dd = DDs();
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&dd_, kDDString, kDDDString, default_dd});
}

double DDVerifier::GetError() const {
    return error_;
}

void DDVerifier::VisualizeHighlights() const {
    for (auto const &hl : highlights_) {
        auto const &col_data = typed_relation_->GetColumnData(hl.GetAttributeIndex());
        RelationalSchema const *col_schema = typed_relation_->GetSchema();
        auto const &pair = hl.GetPairRows();
        LOG_DEBUG("DD does not hold in {} in {} and {} rows with values {}, {}\n",
                  col_schema->GetColumn(hl.GetAttributeIndex())->GetName(), pair.first, pair.second,
                  col_data.GetDataAsString(pair.first), col_data.GetDataAsString(pair.second));
    }
}

std::size_t DDVerifier::GetNumErrorRhs() const {
    return num_error_rhs_;
}

bool DDVerifier::IsColumnMetrizable(model::ColumnIndex const column_index) const {
    model::TypedColumnData const &column = typed_relation_->GetColumnData(column_index);
    model::TypeId const type_id = column.GetTypeId();

    if (type_id == +model::TypeId::kUndefined) {
        throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                    "\" type undefined.");
    }
    if (type_id == +model::TypeId::kMixed) {
        throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                    "\" contains values of different types.");
    }
    return column.GetType().IsMetrizable();
}

double DDVerifier::CalculateDistance(model::ColumnIndex const column_index,
                                     std::pair<std::size_t, std::size_t> const &tuple_pair) const {
    model::TypedColumnData const &column = typed_relation_->GetColumnData(column_index);
    std::byte const *first_value = column.GetValue(tuple_pair.first);
    std::byte const *second_value = column.GetValue(tuple_pair.second);
    auto const &type = static_cast<model::IMetrizableType const &>(column.GetType());
    return type.Dist(first_value, second_value);
}

std::vector<std::pair<std::size_t, std::size_t>> DDVerifier::GetRowsWhereLhsHolds() const {
    std::vector<std::pair<std::size_t, std::size_t>> result;
    for (std::size_t i = 0; i < num_rows_; ++i) {
        for (std::size_t j = i + 1; j < num_rows_; ++j) {
            bool all_lhs_hold = true;
            auto curr_constraint = dd_.left.cbegin();
            for (auto const &column_index : lhs_column_indices_) {
                double const diff = CalculateDistance(column_index, {i, j});
                if (!curr_constraint->constraint.Contains(diff)) {
                    all_lhs_hold = false;
                    break;
                }
                ++curr_constraint;
            }

            if (all_lhs_hold) {
                result.emplace_back(i, j);
            }
        }
    }
    return result;
}

void DDVerifier::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, false);
}

void DDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kDDString});
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
}

unsigned long long DDVerifier::ExecuteInternal() {
    auto elapsed_milliseconds = util::TimedInvoke(&DDVerifier::VerifyDD, this);
    PrintStatistics();
    return elapsed_milliseconds;
}

void DDVerifier::CheckDFOnRhs(std::vector<std::pair<std::size_t, std::size_t>> const &lhs) {
    for (auto const &pair : lhs) {
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

void DDVerifier::VerifyDD() {
    auto get_column_index = [&](auto const &constraint) {
        return typed_relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
    };
    std::ranges::transform(dd_.left, std::back_inserter(lhs_column_indices_), get_column_index);
    std::ranges::transform(dd_.right, std::back_inserter(rhs_column_indices_), get_column_index);
    for (auto const col : lhs_column_indices_) {
        if (!IsColumnMetrizable(col)) {
            std::string message;
            message += std::to_string(col);
            message += " column is not metrizable";
            throw std::invalid_argument(message);
        }
    }
    for (auto const col : rhs_column_indices_) {
        if (!IsColumnMetrizable(col)) {
            std::string message;
            message += std::to_string(col);
            message += " column is not metrizable";
            throw std::invalid_argument(message);
        }
    }
    CheckCorrectnessDd();
    num_rows_ = typed_relation_->GetNumRows();
    num_columns_ = typed_relation_->GetNumColumns();

    std::vector<std::pair<std::size_t, std::size_t>> const lhs = GetRowsWhereLhsHolds();
    CheckDFOnRhs(lhs);
    error_ = lhs.empty() ? 0 : static_cast<double>(num_error_rhs_) / lhs.size();
}

std::vector<Highlight> const &DDVerifier::GetHighlights() const {
    return highlights_;
}

bool DDVerifier::DDHolds() const {
    return !num_error_rhs_;
}

void DDVerifier::PrintStatistics() const {
    if (DDHolds()) {
        LOG_DEBUG("DD holds.");
    } else {
        LOG_DEBUG("DD does not hold.");
        LOG_DEBUG("Number of rhs rows with errors: {}", GetNumErrorRhs());
        LOG_DEBUG("DD error threshold: {}", GetError());
        VisualizeHighlights();
    }
}
}  // namespace algos::dd
