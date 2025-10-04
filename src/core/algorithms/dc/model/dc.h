#pragma once

#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/dc/model/predicate.h"
#include "dc/model/column_operand.h"
#include "dc/model/operator.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace algos::dc {

enum class DCType {
    kOneTuple = 0,  // Each predicate is of form t.A op t.B or t.A op const_value
    kTwoTuples,     // Each predicate is of form s.A op t.B
    kMixed,         // Both kTwoTuples and kOneTuple predicates are in DC
    kAllEquality,   // Each predicate is of form s.A == t.A
    kOneInequality  // DC is kAllEquality except one predicate of form s.A op t.B, op is inequality
};

// @brief Represents a Denial Constraint (DC).
//
// DCs involve comparisons between pairs of rows within a dataset.
// A typical DC example, derived from a Functional Dependency such as A -> B,
// is expressed as: `forall t, s in R: t != s, not (t.A == s.A and t.B != s.B)`
// This denotes that for any pair of rows in the relation, it should not be the case
// that while the values in column "A" are equal, the values in column "B" are unequal.
//
class DC {
private:
    std::vector<Predicate> predicates_;
    bool CheckAllEquality() const;
    bool CheckOneInequality() const;
    bool CheckOneTuple() const;
    bool CheckTwoTuples() const;

public:
    DC(std::vector<Predicate>&& predicates) : predicates_(std::move(predicates)) {};
    DC(std::vector<Predicate> const& predicates) : predicates_(predicates) {};
    DC() = default;

    template <class Iter>
    DC(Iter first, Iter last) : predicates_(first, last){};

    // returns unique columns indices from each Predicate which satisfy the given predicate
    template <class Pred>
    std::vector<Column::IndexType> GetColumnIndicesWithOperator(Pred check) const {
        std::set<Column::IndexType> res;
        for (Predicate const& pred : predicates_) {
            if (check(pred.GetOperator())) {
                dc::ColumnOperand left_operand = pred.GetLeftOperand();
                dc::ColumnOperand right_operand = pred.GetRightOperand();
                if (left_operand.IsVariable()) res.insert(left_operand.GetColumn()->GetIndex());
                if (right_operand.IsVariable()) res.insert(right_operand.GetColumn()->GetIndex());
            }
        }

        return {res.begin(), res.end()};
    }

    std::vector<Column::IndexType> GetColumnIndices() const {
        return GetColumnIndicesWithOperator([](Operator) { return true; });
    }

    template <class Pred>
    std::vector<Predicate> GetPredicates(Pred check) const {
        std::vector<Predicate> res;
        std::copy_if(predicates_.begin(), predicates_.end(), std::back_inserter(res), check);

        return res;
    }

    std::vector<Predicate> const& GetPredicates() const {
        return predicates_;
    };

    DCType GetType() const;

    std::string ToString() const;

    // Convert all two-tuple equality predicates: s.A == t.B -> (s.A <= t.B and s.A >= t.B)
    void ConvertEqualities();
};

}  // namespace algos::dc
