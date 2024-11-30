#pragma once

#include <string>

#include "algorithms/dc/model/column_operand.h"
#include "algorithms/dc/model/operator.h"
#include "model/table/typed_column_data.h"

namespace algos::dc {

//  @brief Represents a predicate for Denial Constraint (DC).
//
//  A predicate (e.g., t.A == s.A) comprises three elements: the column
//  operand from the first tuple ("t.A"), the comparison operator ("=="),
//  and the column operand from the second tuple ("s.A").
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

    bool IsCrossColumn() const noexcept {
        return l_.GetColumn() != r_.GetColumn();
    }

    bool IsCrossTuple() const noexcept {
        return l_.IsFirstTuple() != r_.IsFirstTuple();
    }

    bool HasSameOperandsAs(Predicate const& rhs) const noexcept {
        return l_ == rhs.GetLeftOperand() && r_ == rhs.GetRightOperand();
    }

    std::string ToString() const {
        return l_.ToString() + " " + op_.ToString() + " " + r_.ToString();
    }
};

}  // namespace algos::dc
