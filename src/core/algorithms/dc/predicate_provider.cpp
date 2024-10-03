#include "dc/predicate_provider.h"

#include "dc/column_operand.h"
#include "dc/operator.h"

namespace algos::fastadc {

PredicatePtr PredicateProvider::GetPredicate(Operator const& op, ColumnOperand const& left,
                                             ColumnOperand const& right) {
    auto [iter, _] = predicates_[op][left].try_emplace(right, op, left, right);
    return &iter->second;
}

}  // namespace algos::fastadc
