#include "algorithms/dc/parser/dc_parser.h"

#include <ranges>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "algorithms/dc/model/dc.h"
#include "algorithms/dc/model/operator.h"
#include "algorithms/dc/model/predicate.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/typed_column_data.h"

namespace mo = model;

namespace algos::dc {

DCParser::DCParser(std::string dc_string, ColumnLayoutRelationData const* relation,
                   std::vector<model::TypedColumnData> const& data)
    : relation_(relation),
      data_(data),
      dc_string_(std::move(dc_string)),
      has_next_predicate_(true),
      cur_(0) {
    str_operators_.reserve(str_operators_.size());
    for (auto const& [frozen_str, _] : Operator::kStringToOperatorType) {
        std::string str_op = frozen_str.data();
        str_operators_.emplace_back(std::move(str_op));
    }
};

// We're given a following string, for example:
// " !( s.Salary <  s.Salary and  t.State  !=  t.State and t.TaxRate == 0.1 ) "
//
// 1. First we remove trailing and leading spaces
// 2. Then find and remove negation and brackets
// 3. After all split predicates with " and "
// 4. For each predicate also remove surrounding spaces
// 5. Then split each predicate into parts and create Predicate object
// 6. Create a DC object using extracted Predicates
//
// predicate that consists of two constant values is not allowed: 2.7 < 3.1415
//
DC DCParser::Parse() {
    boost::trim(dc_string_);

    if (dc_string_.front() != '!') throw std::invalid_argument("Missing logic negation sign");
    dc_string_.erase(dc_string_.begin());
    boost::trim_left(dc_string_);

    if (dc_string_.front() != '(' or dc_string_.back() != ')')
        throw std::invalid_argument("Missing parenthesis");

    dc_string_.erase(dc_string_.begin());
    dc_string_.erase(std::prev(dc_string_.end()));

    if (dc_string_.empty()) throw std::invalid_argument("Empty DC is not allowed");

    std::vector<Predicate> res;
    while (has_next_predicate_) {
        res.push_back(GetNextPredicate());
    }

    return {std::move(res)};
}

Predicate DCParser::GetNextPredicate() {
    size_t ind = dc_string_.find(kSep, cur_);
    if (ind == std::string::npos) has_next_predicate_ = false;

    std::string str_pred = dc_string_.substr(cur_, ind - cur_);
    cur_ = ind + kSep.size();

    return ConvertToPredicate(std::move(str_pred));
}

Predicate DCParser::ConvertToPredicate(std::string const& pred) {
    size_t ind;
    auto check = [&ind, &pred](std::string const& op) {
        std::string spaced_op = ' ' + op + ' ';
        ind = pred.find(spaced_op);
        return ind != std::string::npos;
    };

    auto it = std::ranges::find_if(str_operators_, check);
    if (it == str_operators_.end()) throw std::invalid_argument("Missing permissible operator");

    std::string str_operator = *it;
    size_t right_start = ind + str_operator.size() + 1;
    auto str_left_operand = pred.substr(0, ind);
    auto str_right_operand = pred.substr(right_start);

    boost::trim(str_left_operand);
    boost::trim(str_right_operand);

    auto [left_operand, right_operand] = GetOperands(str_left_operand, str_right_operand);

    return {std::move(str_operator), std::move(left_operand), std::move(right_operand)};
}

std::pair<ColumnOperand, ColumnOperand> DCParser::GetOperands(std::string str_left_op,
                                                              std::string str_right_op) {
    std::optional<ColumnOperand> left_op, right_op;

    if (IsVarOperand(str_left_op)) {
        RemoveEscaping(str_left_op);
        left_op = ConvertToVariableOperand(str_left_op);
    }

    if (IsVarOperand(str_right_op)) {
        RemoveEscaping(str_right_op);
        right_op = ConvertToVariableOperand(str_right_op);
    }

    if (!left_op.has_value() and !right_op.has_value())
        throw std::invalid_argument("Pure constant predicate is not allowed");

    if (!left_op.has_value()) {
        left_op = ColumnOperand(str_left_op, right_op->GetType());
    } else if (!right_op.has_value()) {
        right_op = ColumnOperand(str_right_op, left_op->GetType());
    }

    return {std::move(*left_op), std::move(*right_op)};
}

void DCParser::RemoveEscaping(std::string& str) const {
    size_t ind = str.find_first_of('\\');
    if (ind != std::string::npos) str.erase(ind, 1);
}

bool DCParser::IsVarOperand(std::string op) const {
    return op.starts_with("t.") or op.starts_with("s.");
}

ColumnOperand DCParser::ConvertToVariableOperand(std::string const& operand) const {
    dc::Tuple tuple = operand.front() == 't' ? dc::Tuple::kT : dc::Tuple::kS;
    std::string name = operand.substr(2);
    std::vector<std::unique_ptr<Column>> const& cols = relation_->GetSchema()->GetColumns();
    std::vector<std::unique_ptr<Column>>::const_iterator it;
    if (!cols.front()->GetName().empty()) {  // Has header
        auto pred = [&name](auto const& col) { return name == col->GetName(); };
        it = std::ranges::find_if(cols, pred);
    }

    Column* column = nullptr;
    if (it == cols.end()) {
        try {
            std::string str_ind = operand.substr(2);
            size_t ind = static_cast<mo::ColumnIndex>(std::stoi(str_ind));
            column = cols[ind].get();
        } catch (std::exception const& e) {
            throw std::invalid_argument("Unknown column index or name");
        }
    } else {
        column = it->get();
    }

    size_t col_ind = column->GetIndex();
    return {column, tuple, &data_[col_ind].GetType()};
}

}  // namespace algos::dc
