#pragma once

#include <string>
#include <variant>

#include "algorithms/dc/model/column_operand.h"
#include "algorithms/dc/model/operator.h"
#include "algorithms/dc/model/tuple.h"
#include "model/table/typed_column_data.h"

namespace algos::dc {

// @brief Represents a predicate for Denial Constraint (DC).
//
// A predicate (e.g., t.A == s.A) comprises three elements: the column
// operand from the first tuple ("t.A"), the comparison operator ("=="),
// and the column operand from the second tuple ("s.A").
//
class Predicate {
private:
    Operator op_;
    ColumnOperand l_;
    ColumnOperand r_;

public:
    Predicate(Operator const& op, ColumnOperand const& l, ColumnOperand const& r) noexcept
        : op_(op), l_(l), r_(r) {}

    Operator GetOperator() const noexcept {
        return op_;
    }

    ColumnOperand GetLeftOperand() const noexcept {
        return l_;
    }

    ColumnOperand GetRightOperand() const noexcept {
        return r_;
    }

    // Is used to get a variable operand in a constant predicate
    ColumnOperand const& GetVariableOperand() const {
        return l_.IsVariable() ? l_ : r_;
    }

    bool IsCrossColumn() const {
        return IsVariable() and l_.GetColumn() != r_.GetColumn();
    }

    bool IsOneColumn() const {
        return IsVariable() and l_.GetColumn() == r_.GetColumn();
    }

    bool IsCrossTuple() const {
        return IsVariable() and l_.GetTuple() != r_.GetTuple();
    }

    bool IsOneTuple() const {
        return IsConstant() or !IsCrossTuple();
    }

    bool IsConstant() const {
        return l_.IsConstant() or r_.IsConstant();
    }

    bool IsVariable() const {
        return !IsConstant();
    }

    std::string ToString() const {
        return l_.ToString() + " " + op_.ToString() + " " + r_.ToString();
    }

    Tuple GetTuple() const {
        if (IsConstant()) return GetVariableOperand().GetTuple();
        if (IsCrossTuple()) return Tuple::kMixed;
        return l_.GetTuple();
    }
};

}  // namespace algos::dc
