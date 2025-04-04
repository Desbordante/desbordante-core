#include "algorithms/dd/dd_verifier/dd_verifier.h"

#include <chrono>
#include <cstddef>
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

void DDVerifier::VisualizeHighlights() {
    for (auto const &[col_index, pair] : highlights_) {
        auto const &col_data = typed_relation_->GetColumnData(col_index);
        auto const col_schema = typed_relation_->GetSchema();
        LOG(DEBUG) << "DD not Holds in " << col_schema->GetColumn(col_index)->GetName() << " in "
                   << pair.first << " and " << pair.second << " rows with values "
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
    if (column.IsNull(tuple_pair.first) || column.IsNull(tuple_pair.second)) {
        throw std::runtime_error("Some of the value coordinates are nulls.");
    }
    if (column.IsEmpty(tuple_pair.first) || column.IsEmpty(tuple_pair.second)) {
        throw std::runtime_error("Some of the value coordinates are empty.");
    }
    double dif = 0;
    std::byte const *first_value = column.GetValue(tuple_pair.first);
    std::byte const *second_value = column.GetValue(tuple_pair.second);
    auto const &type = static_cast<model::IMetrizableType const &>(column.GetType());
    dif = type.Dist(first_value, second_value);
    return dif;
}

std::vector<std::pair<int, int>> DDVerifier::GetRowsWhereLhsHolds() const {
    std::vector<std::pair<int, int>> result;
    std::vector<model::ColumnIndex> columns;
    for (auto const &constraint : dd_.left) {
        model::ColumnIndex column_index =
                typed_relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    auto curr_constraint = dd_.left.cbegin();
    if (IsColumnMetrizable(columns[0])) {
            for (std::size_t i = 0; i < num_rows_; i++) {
                for (std::size_t j = i + 1; j < num_rows_; j++) {
                    if (auto const dif = CalculateDistance(columns[0], {i, j});
                        curr_constraint->constraint.Contains(dif)) {
                        result.emplace_back(i, j);
                    }
                }
            }
    }
    ++curr_constraint;
    for (std::size_t i = 1; i < columns.size(); i++) {
        if (IsColumnMetrizable(columns[i])) {
            for (std::size_t j = 0; i < result.size(); j++) {
                if (double const dif = CalculateDistance(columns[i], result[j]);
                    !curr_constraint->constraint.Contains(dif)) {
                    result.erase(result.cbegin() + static_cast<int>(j));
                    --j;
                }
            }
            ++curr_constraint;
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

unsigned long long DDVerifier::ExecuteInternal() {
    num_rows_ = typed_relation_->GetNumRows();
    num_columns_ = typed_relation_->GetNumColumns();

    auto start_time = std::chrono::system_clock::now();

    VerifyDD();

    PrintStatistics();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    return elapsed_milliseconds.count();
}

void DDVerifier::CheckDFOnRhs(std::vector<std::pair<int, int>> const &lhs) {
    std::vector<model::ColumnIndex> columns;
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
                highlights_.emplace_back(column_index, pair);
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
    std::vector<std::pair<int, int>> const lhs = GetRowsWhereLhsHolds();
    CheckDFOnRhs(lhs);
    if (lhs.empty()) {
        error_ = 0;
    } else {
        error_ = static_cast<double>(num_error_rhs_) / static_cast<double>(lhs.size());
    }
}

std::vector<std::pair<std::size_t, std::pair<int, int>>> DDVerifier::GetHighlights() const {
    return highlights_;
}

bool DDVerifier::DDHolds() const {
    return !num_error_rhs_;
}

void DDVerifier::PrintStatistics() {
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
