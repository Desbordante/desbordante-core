#pragma once

#include <string>
#include <variant>

#include "core/algorithms/dc/model/column_operand.h"
#include "core/algorithms/dc/model/component.h"
#include "core/algorithms/dc/model/operator.h"
#include "core/algorithms/dc/model/tuple.h"
#include "core/model/table/typed_column_data.h"

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

    // Transform a predicate to the following form: s.A op t.B
    void Canonize() {
        if (!IsCrossTuple()) return;
        if (l_.GetTuple() != dc::Tuple::kS) {
            std::swap(l_, r_);
            op_.Inverse();
        }
    }

    bool Eval(std::vector<std::byte const*> const& row) const {
        assert(IsOneTuple());
        dc::Component l_comp = l_.Eval(row);
        dc::Component r_comp = r_.Eval(row);
        return dc::Component::Eval(l_comp, r_comp, op_);
    }

    bool Eval(std::vector<std::byte const*> const& s_row,
              std::vector<std::byte const*> const& t_row) const {
        assert(IsCrossTuple());
        auto const& l_row = (l_.GetTuple() == Tuple::kT) ? t_row : s_row;
        auto const& r_row = (r_.GetTuple() == Tuple::kT) ? t_row : s_row;
        dc::Component l_comp = l_.Eval(l_row);
        dc::Component r_comp = r_.Eval(r_row);
        return dc::Component::Eval(l_comp, r_comp, op_);
    }
};

}  // namespace algos::dc
