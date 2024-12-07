#include "algorithms/dc/model/dc.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/dc/model/operator.h"
#include "algorithms/dc/model/predicate.h"

namespace algos::dc {

bool DC::CheckAllEquality() const {
    for (Predicate const& pred : predicates_) {
        Operator op = pred.GetOperator();
        if (pred.IsCrossColumn() or !pred.IsCrossTuple() or op != OperatorType::kEqual)
            return false;
    }

    return true;
}

bool DC::CheckOneInequality() const {
    size_t count_eq = 0, count_ineq = 0;

    for (Predicate const& pred : predicates_) {
        Operator op = pred.GetOperator();

        if (op == OperatorType::kEqual and !pred.IsCrossColumn()) {
            count_eq++;
        } else if (op != OperatorType::kEqual and op != OperatorType::kUnequal and
                   pred.IsCrossTuple()) {
            count_ineq++;
        }
    }

    return count_eq + count_ineq == predicates_.size() && count_ineq == 1;
}

bool DC::CheckOneTuple() const {
    return std::none_of(predicates_.begin(), predicates_.end(),
                        std::mem_fn(&Predicate::IsCrossTuple));
}

bool DC::CheckTwoTuples() const {
    return std::all_of(predicates_.begin(), predicates_.end(),
                       std::mem_fn(&Predicate::IsCrossTuple));
}

DCType DC::GetType() const {
    if (CheckAllEquality())
        return DCType::kAllEquality;
    else if (CheckOneInequality())
        return DCType::kOneInequality;
    else if (CheckOneTuple())
        return DCType::kOneTuple;
    else if (CheckTwoTuples())
        return DCType::kTwoTuples;
    else
        return DCType::kMixed;
}

std::string DC::ToString() const {
    if (predicates_.empty()) return {};

    static constexpr char const* kNot = "!";
    static constexpr char const* kAnd = " and ";

    std::stringstream ss;
    ss << kNot << '(' << predicates_.front().ToString();
    for (auto pred = std::next(predicates_.begin()); pred != predicates_.end(); ++pred) {
        ss << kAnd << pred->ToString();
    }
    ss << ')';

    return ss.str();
}

}  // namespace algos::dc