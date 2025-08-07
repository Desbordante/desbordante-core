#include "algorithms/dd/dd_verifier/dd_verifier.h"

#include <chrono>
#include <cstddef>
#include <timed_invoke.h>
#include <vector>

#include <easylogging++.h>

#include "config/descriptions.h"
#include "config/names.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "model/table/vertical.h"

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
        auto const col_schema = typed_relation_->GetSchema();
        auto const &pair = hl.GetPairRows();
        LOG(DEBUG) << "DD not Holds in " << col_schema->GetColumn(hl.GetAttributeIndex())->GetName()
                   << " in " << pair.first << " and " << pair.second << " rows with values "
                   << col_data.GetDataAsString(pair.first) << ", "
                   << col_data.GetDataAsString(pair.second) << '\n';
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

std::vector<std::pair<int, int>> DDVerifier::GetRowsWhereLhsHolds() const {
    std::vector<std::pair<int, int>> result;
    std::vector<model::ColumnIndex> columns;
    columns.reserve(dd_.left.size());
    for (auto const &constraint : dd_.left) {
        model::ColumnIndex column_index =
                typed_relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    auto curr_constraint = dd_.left.cbegin();
    for (std::size_t i = 0; i < num_rows_; i++) {
        for (std::size_t j = i + 1; j < num_rows_; j++) {
            if (auto const dif = CalculateDistance(columns[0], {i, j});
                curr_constraint->constraint.Contains(dif)) {
                result.emplace_back(i, j);
            }
        }
    }
    ++curr_constraint;
    for (std::size_t i = 1; i < columns.size(); i++) {
        std::vector<std::pair<int, int>> new_result;
        for (auto const &pair : result) {
            if (double const dif = CalculateDistance(columns[i], pair);
                curr_constraint->constraint.Contains(dif)) {
                new_result.emplace_back(pair);
            }
        }
        result = std::move(new_result);
        ++curr_constraint;
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

void DDVerifier::CheckDFOnRhs(std::vector<std::pair<int, int>> const &lhs) {
    std::vector<model::ColumnIndex> columns;
    columns.reserve(dd_.right.size());
    for (auto const &dd : dd_.right) {
        model::ColumnIndex column_index =
                typed_relation_->GetSchema()->GetColumn(dd.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    for (auto const &pair : lhs) {
        auto curr_constraint = dd_.right.cbegin();
        bool is_error = false;
        for (auto const column_index : columns) {
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
    std::vector<model::ColumnIndex> columns;
    columns.reserve(dd_.left.size() + dd_.right.size());
    for (auto const &constraint : dd_.left) {
        model::ColumnIndex column_index =
                typed_relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    for (auto const &constraint : dd_.right) {
        model::ColumnIndex column_index =
                typed_relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    for (auto const col : columns) {
        if (!IsColumnMetrizable(col)) {
            throw std::invalid_argument("One of the columns is not metrizable");
        }
    }
    CheckCorrectnessDd();
    num_rows_ = typed_relation_->GetNumRows();
    num_columns_ = typed_relation_->GetNumColumns();

    std::vector<std::pair<int, int>> const lhs = GetRowsWhereLhsHolds();
    CheckDFOnRhs(lhs);
    error_ = lhs.empty() ? 0 : static_cast<double>(num_error_rhs_) / lhs.size();
}

std::vector<Highlight> DDVerifier::GetHighlights() const {
    return highlights_;
}

bool DDVerifier::DDHolds() const {
    return !num_error_rhs_;
}

void DDVerifier::PrintStatistics() const {
    if (DDHolds()) {
        LOG(DEBUG) << "DD holds.";
    } else {
        LOG(DEBUG) << "DD does not hold.";
        LOG(DEBUG) << "Number of rhs rows with errors: " << GetNumErrorRhs();
        LOG(DEBUG) << "DD error threshold: " << GetError();
        VisualizeHighlights();
    }
}
}  // namespace algos::dd
