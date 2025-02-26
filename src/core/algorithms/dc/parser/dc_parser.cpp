#include "algorithms/dc/parser/dc_parser.h"

#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "algorithms/dc/model/dc.h"
#include "algorithms/dc/model/operator.h"
#include "algorithms/dc/model/predicate.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/typed_column_data.h"

namespace mo = model;

namespace {

using namespace algos::dc;

std::unique_ptr<ColumnOperand> GetOperand(ColumnOperand const& operand, std::string str_val) {
    mo::Type const* type = operand.GetType();
    return std::make_unique<ColumnOperand>(str_val, type);
}

}  // namespace

namespace algos::dc {

DCParser::DCParser(std::string dc_string, ColumnLayoutRelationData const* relation,
                   std::vector<model::TypedColumnData> const& data)
    : relation_(relation),
      data_(data),
      dc_string_(std::move(dc_string)),
      has_next_predicate_(true),
      cur_(0) {
    str_operators_.reserve(str_operators_.size());
    for (std::pair<frozen::string const, algos::dc::OperatorType> const& op_it :
         Operator::kStringToOperatorType) {
        std::string str_op = op_it.first.data();
        str_operators_.emplace_back(std::move(str_op));
    }
};

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
        Predicate pred = GetNextPredicate();
        res.push_back(pred);
    }

    return {res};
}

Predicate DCParser::GetNextPredicate() {
    size_t ind = dc_string_.find(sep_, cur_);
    if (ind == std::string::npos) has_next_predicate_ = false;

    std::string str_pred = dc_string_.substr(cur_, ind - cur_);
    cur_ = ind + sep_.size();

    return ConvertToPredicate(std::move(str_pred));
}

Predicate DCParser::ConvertToPredicate(std::string pred) {
    size_t ind;
    std::string str_operator;
    for (std::string const& op : str_operators_) {
        std::string spaced_op = ' ' + op + ' ';
        ind = pred.find(spaced_op);
        if (ind != std::string::npos) {
            str_operator = op;
            break;
        }
    }

    if (ind == std::string::npos) throw std::invalid_argument("Missing permissible operator");

    size_t right_start = ind + str_operator.size() + 1;
    auto str_left_operand = std::string(pred, 0, ind);
    auto str_right_operand = std::string(pred, right_start, pred.size() - right_start + 1);

    boost::trim(str_left_operand);
    boost::trim(str_right_operand);

    auto [left_operand, right_operand] = GetOperands(str_left_operand, str_right_operand);

    return {std::move(str_operator), std::move(left_operand), std::move(right_operand)};
}

std::pair<ColumnOperand, ColumnOperand> DCParser::GetOperands(std::string str_left_op,
                                                              std::string str_right_op) {
    std::unique_ptr<ColumnOperand> left_op, right_op;

    if (IsVarOperand(str_left_op)) {
        RemoveEscaping(str_left_op);
        ColumnOperand operand = ConvertToVariableOperand(str_left_op);
        left_op = std::make_unique<dc::ColumnOperand>(operand);
    }

    if (IsVarOperand(str_right_op)) {
        RemoveEscaping(str_right_op);
        ColumnOperand operand = ConvertToVariableOperand(str_right_op);
        right_op = std::make_unique<dc::ColumnOperand>(operand);
    }

    if (!left_op and !right_op)
        throw std::invalid_argument("Pure constant predicate is not allowed");

    if (!left_op) {
        left_op = GetOperand(*right_op.get(), str_left_op);
    } else if (right_op == nullptr) {
        right_op = GetOperand(*left_op.get(), str_right_op);
    }

    return {std::move(*left_op), std::move(*right_op)};
}

void DCParser::RemoveEscaping(std::string& str) const {
    size_t ind = str.find_first_of('\\');
    if (ind != std::string::npos) str.erase(ind, 1);
}

bool DCParser::IsVarOperand(std::string op) const {
    return op.find("t.") == 0 or op.find("s.") == 0;
}

ColumnOperand DCParser::ConvertToVariableOperand(std::string operand) const {
    Column* column = nullptr;
    bool is_first_tuple = operand.front() == 't';
    std::string name(operand.begin() + 2, operand.end());
    std::vector<std::unique_ptr<Column>> const& cols = relation_->GetSchema()->GetColumns();
    if (!cols.front()->GetName().empty()) {  // Has header
        for (auto const& col : cols) {
            if (name == col->GetName()) {
                column = col.get();
                break;
            }
        }
    }

    if (column == nullptr) {
        try {
            std::string str_ind(operand.begin() + 2, operand.end());
            size_t ind = static_cast<mo::ColumnIndex>(std::stoi(str_ind));
            column = cols[ind].get();
        } catch (std::exception const& e) {
            throw std::invalid_argument("Unknown column index or name");
        }
    }

    size_t col_ind = column->GetIndex();
    return {column, is_first_tuple, &data_[col_ind].GetType()};
}

}  // namespace algos::dc
