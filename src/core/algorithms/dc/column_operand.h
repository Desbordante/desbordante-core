#pragma once

#include "table/column.h"

namespace algos {

namespace dc {

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
 * ("="), and the column operand from the second tuple ("s.A"). The `ColumnOperand` class
 * encapsulates the column operand part of a predicate, such as "t.A" or "s.A".
 *
 * The class distinguishes between operands derived from the first tuple (t) and those
 * from the second tuple (s) using a boolean flag `tuple_`, where `true` indicates an
 * operand from the first tuple (t), and `false` indicates an operand from the second
 * tuple (s).
 */
class ColumnOperand {
private:
    Column const* column_;
    bool tuple_;

public:
    ColumnOperand(Column const* column, bool tuple) : column_(column), tuple_(tuple) {}

    bool operator==(ColumnOperand const& rhs) const {
        return column_ == rhs.column_ && tuple_ == rhs.tuple_;
    }

    Column const* GetColumn() const {
        return column_;
    }

    bool GetTuple() const {
        return tuple_;
    }

    std::string ToString() const {
        return (tuple_ ? "t." : "s.") + column_->GetName();
    }
};

}  // namespace dc

}  // namespace algos