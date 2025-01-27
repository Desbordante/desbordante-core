#include "predicate.h"

#include <algorithm>

#include "dc/FastADC/model/column_operand.h"
#include "dc/FastADC/model/operator.h"
#include "dc/FastADC/providers/predicate_provider.h"
#include "model/table/column.h"
#include "model/table/typed_column_data.h"

namespace algos::fastadc {

PredicatePtr GetPredicateByType(PredicatesSpan predicates, OperatorType type) {
    auto it = std::find_if(predicates.begin(), predicates.end(),
                           [type](PredicatePtr p) { return p->GetOperator() == type; });

    return it == predicates.end() ? nullptr : *it;
}

bool Predicate::Satisfies(std::vector<model::TypedColumnData>& col_data, size_t t, size_t s) const {
    model::TypedColumnData const& lhs = col_data[l_.GetColumn()->GetIndex()];
    model::TypedColumnData const& rhs = col_data[r_.GetColumn()->GetIndex()];

    model::Type const& lhs_type = lhs.GetType();
    model::Type const& rhs_type = rhs.GetType();
    assert(lhs_type == rhs_type && "Column types do not match.");

    model::Type const& type = lhs_type;

    std::byte const* l_val = lhs.GetValue(l_.GetTuple() == +ColumnOperandTuple::t ? t : s);
    std::byte const* r_val = rhs.GetValue(r_.GetTuple() == +ColumnOperandTuple::t ? t : s);

    return op_.Eval(l_val, r_val, type);
}

PredicatePtr Predicate::GetSymmetric(PredicateProvider* provider) const {
    if (!symmetric_) {
        symmetric_ = provider->GetPredicate(op_.GetSymmetric(), r_, l_);
    }
    return symmetric_;
}

PredicatePtr Predicate::GetInvTS(PredicateProvider* provider) const {
    if (!inv_TS_) {
        inv_TS_ = provider->GetPredicate(op_, l_.GetInvTS(), r_.GetInvTS());
    }
    return inv_TS_;
}

PredicatePtr Predicate::GetInverse(PredicateProvider* provider) const {
    if (!inverse_) {
        inverse_ = provider->GetPredicate(op_.GetInverse(), l_, r_);
    }
    return inverse_;
}

std::vector<PredicatePtr> const& Predicate::GetImplications(PredicateProvider* provider) const {
    if (implications_.empty()) {
        auto op_implications = op_.GetImplications();
        for (Operator op_implication : op_implications) {
            implications_.push_back(provider->GetPredicate(op_implication, l_, r_));
        }
    }
    return implications_;
}

}  // namespace algos::fastadc
