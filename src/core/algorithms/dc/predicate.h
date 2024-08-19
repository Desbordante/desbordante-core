#pragma once

#include "column_operand.h"
#include "operator.h"
#include "table/typed_column_data.h"

namespace model {

/**
 * @brief Represents a predicate for Denial Constraint (DC).
 *
 * DCs involve comparisons between pairs of rows within a dataset.
 * A typical DC example, derived from a Functional Dependency such as A -> B,
 * is expressed as: `forall t, s in r, not (t.A = s.A and t.B != s.B)`
 * This denotes that for any pair of rows in the relation, it should not be the case
 * that while the values in column "A" are equal, the values in column "B" are unequal.
 *
 * A predicate in this context (e.g., t.A == s.A) comprises three elements to be fully
 * represented: the column operand from the first tuple ("t.A"), the comparison operator
 * ("="), and the column operand from the second tuple ("s.A").
 */
class Predicate {
private:
    Operator op_;
    ColumnOperand l_;
    ColumnOperand r_;

public:
    Predicate(Operator const& op, ColumnOperand const& l, ColumnOperand const& r)
        : op_(op), l_(l), r_(r) {}

    Operator GetOperator() const {
        return op_;
    }

    ColumnOperand GetLeftOperand() const {
        return l_;
    }

    ColumnOperand GetRightOperand() const {
        return r_;
    }

    bool IsCrossColumn() const {
        return l_.GetColumn() != r_.GetColumn();
    }

    bool HasSameOperandsAs(Predicate const& rhs) const {
        return l_ == rhs.GetLeftOperand() && r_ == rhs.GetRightOperand();
    }

    std::string ToString() const {
        return l_.ToString() + " " + op_.ToString() + " " + r_.ToString();
    }
};

}  // namespace model
