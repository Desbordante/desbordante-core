#pragma once

#include "column_operand.h"
#include "operator.h"

namespace model {

class Predicate;
using PredicatePtr = Predicate const*;

class Predicate {
private:
    Operator op_;
    ColumnOperand l_;
    ColumnOperand r_;

public:
    Predicate(Operator const& op, ColumnOperand const& l, ColumnOperand const& r)
        : op_(op), l_(l), r_(r) {}
};

}  // namespace model
