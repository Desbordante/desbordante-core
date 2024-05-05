#include "dc.h"

namespace model {

std::string DC::DCToString() {
    std::string NOT = "!";
    std::string AND = " and ";

    std::string res = predicates_[0].ToString();
    for (size_t i = 1; i < predicates_.size(); i++) {
        res += AND + predicates_[i].ToString();
    }

    return NOT + "(" + res + ")";
}

std::vector<unsigned> DC::GetColumnIndicesWithOperator(Operator op) {
    std::vector<unsigned> res;
    for (Predicate pred : predicates_) {
        if (pred.GetOperator() == op) {
            auto leftInd = pred.GetLeftOperand().GetColumn()->GetIndex();
            auto rightInd = pred.GetRightOperand().GetColumn()->GetIndex();
            res.insert(res.end(), {leftInd, rightInd});
        }
    }

    return res;
}

}  // namespace model