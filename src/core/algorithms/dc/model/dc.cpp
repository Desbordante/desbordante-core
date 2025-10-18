#include "algorithms/dc/model/dc.h"

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "algorithms/dc/model/operator.h"
#include "algorithms/dc/model/predicate.h"

namespace algos {
namespace dc {
enum class Tuple;
}  // namespace dc
}  // namespace algos

namespace algos::dc {

bool DC::CheckAllEquality() const {
    auto check = [](auto const& pred) {
        return pred.GetOperator() == dc::OperatorType::kEqual and pred.IsCrossTuple() and
               pred.IsOneColumn();
    };

    return std::ranges::all_of(predicates_, check);
}

bool DC::CheckOneInequality() const {
    size_t count_eq = 0, count_ineq = 0;
    for (Predicate const& pred : predicates_) {
        if (pred.IsConstant()) return false;

        Operator op = pred.GetOperator();
        if (op == OperatorType::kEqual and pred.IsOneColumn()) {
            count_eq++;
        } else if (op != OperatorType::kEqual and op != OperatorType::kUnequal and
                   pred.IsCrossTuple()) {
            count_ineq++;
        }
    }

    return count_eq + count_ineq == predicates_.size() && count_ineq == 1;
}

bool DC::CheckOneTuple() const {
    ColumnOperand operand = predicates_.front().GetVariableOperand();
    dc::Tuple tuple = operand.GetTuple();

    auto check = [tuple](Predicate const& pred) {
        return (pred.IsConstant() or pred.IsOneTuple()) and
               pred.GetVariableOperand().GetTuple() == tuple;
    };

    return std::ranges::all_of(predicates_, check);
}

bool DC::CheckTwoTuples() const {
    auto check = [](Predicate const& pred) { return pred.IsVariable() and pred.IsCrossTuple(); };

    return std::ranges::all_of(predicates_, check);
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

    static constexpr std::string_view kNot = "!";
    static constexpr std::string_view kAnd = " and ";

    std::stringstream ss;
    ss << kNot << '(' << predicates_.front().ToString();
    for (Predicate const& pred : predicates_ | std::views::drop(1)) {
        ss << kAnd << pred.ToString();
    }
    ss << ')';

    return ss.str();
}

void DC::ConvertEqualities() {
    std::vector<dc::Predicate> res;
    res.reserve(predicates_.size() * 2);
    for (auto const& pred : predicates_) {
        auto left = pred.GetLeftOperand();
        auto right = pred.GetRightOperand();
        if (pred.IsVariable() and pred.IsCrossColumn() and pred.IsCrossTuple() and
            pred.GetOperator().GetType() == dc::OperatorType::kEqual) {
            res.emplace_back(dc::OperatorType::kLessEqual, left, right);
            res.emplace_back(dc::OperatorType::kGreaterEqual, left, right);
        } else {
            res.emplace_back(pred);
        }
    }
    res.shrink_to_fit();
    predicates_ = std::move(res);
}

}  // namespace algos::dc
