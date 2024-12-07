#pragma once

#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/dc/model/predicate.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace algos::dc {

enum class DCType {
    kOneTuple = 0,  // Each predicate is of form s.A op s.B or t.A op t.B
    kTwoTuples,     // Each predicate is of form s.A op t.B
    kMixed,         // Both kTwoTuples and kOneTuple predicates are in DC
    kAllEquality,   // All predicates are of form s.A == t.A
    kOneInequality  // DC is kAllEquality except one predicate of form s.A op t.B, op is inequality
};

// @brief Represents a Denial Constraint (DC).
//
// DCs involve comparisons between pairs of rows within a dataset.
// A typical DC example, derived from a Functional Dependency such as A -> B,
// is expressed as: `forall t, s in R, not (t.A == s.A and t.B != s.B)`
// This denotes that for any pair of rows in the relation, it should not be the case
// that while the values in column "A" are equal, the values in column "B" are unequal.
//
// Thus DC is simply represented as a vector of Predicates.
// The predicates in this context are 't.A == s.A' and 't.B != s.B'.
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

    DCType GetType() const;

    // returns unique columns indices from each Predicate which satisfy the given predicate
    template <class Pred>
    std::vector<Column::IndexType> GetColumnIndicesWithOperator(Pred check) const {
        std::set<Column::IndexType> res;
        for (Predicate const& pred : predicates_) {
            if (check(pred.GetOperator())) {
                Column::IndexType left_ind = pred.GetLeftOperand().GetColumn()->GetIndex();
                Column::IndexType right_ind = pred.GetRightOperand().GetColumn()->GetIndex();
                res.insert({left_ind, right_ind});
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

    std::string ToString() const;

    std::vector<Predicate> const& GetPredicates() const {
        return predicates_;
    };
};

}  // namespace algos::dc
