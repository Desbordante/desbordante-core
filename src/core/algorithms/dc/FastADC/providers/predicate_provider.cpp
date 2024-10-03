#include "predicate_provider.h"

namespace algos::fastadc {

PredicatePtr PredicateProvider::GetPredicate(Operator const& op, ColumnOperand const& left,
                                             ColumnOperand const& right) {
    auto [iter, _] = predicates_[op][left].try_emplace(right, op, left, right);
    return &iter->second;
}

}  // namespace algos::fastadc
