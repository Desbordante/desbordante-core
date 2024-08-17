#pragma once

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "model/table/column.h"
#include "model/table/vertical.h"
#include "predicate.h"

namespace algos {

namespace dc {

// TODO: Add constant dc
enum class DCType {
    kOneTuple = 0,  // Each predicate is of form s.A op s.B or t.A op t.B
    kTwoTuples,     // Each predicate is of form s.A op t.B
    kMixed,         // Both kTwoTuples and kOneTuple predicates are in DC
    kAllEquality,   // All predicates are of form s.A == t.A
    kOneInequality  // DC is kAllEquality except one predicate of form s.A op t.B
};

class DC {
    std::vector<Predicate> predicates_;

public:
    DC(std::vector<Predicate>&& predicates) : predicates_(std::move(predicates)) {};
    DC(std::vector<Predicate> const& predicates) : predicates_(predicates) {};
    DC() = default;

    template <class Iter>
    DC(Iter first, Iter last) {
        while (first != last) predicates_.push_back(*(first++));
    }

    // returns unique columns indices from each Predicate which satisfy the given predicate
    template <class Pred>
    std::vector<uint> GetColumnIndicesWithOperator(Pred check) const {
        std::set<uint> res;
        for (Predicate const& pred : predicates_) {
            if (check(pred.GetOperator())) {
                Column::IndexType left_ind = pred.GetLeftOperand().GetColumn()->GetIndex();
                Column::IndexType right_ind = pred.GetRightOperand().GetColumn()->GetIndex();
                res.insert({left_ind, right_ind});
            }
        }

        return std::vector(res.begin(), res.end());
    }

    std::vector<uint> GetColumnIndices() const {
        return GetColumnIndicesWithOperator([](Operator) { return true; });
    }

    // returns all predicates satisfying the given predicate
    template <class Pred>
    std::vector<Predicate> GetPredicates(Pred check) const {
        std::vector<Predicate> res;
        for (Predicate const& pred : predicates_) {
            if (check(pred)) {
                res.push_back(pred);
            }
        }

        return res;
    }

    std::string DCToString() const {
        std::string const k_not = "!";
        std::string const k_and = " and ";

        std::string res;
        for (size_t i = 0; i < predicates_.size(); i++) {
            res += k_and + predicates_[i].ToString();
        }

        return k_not + "(" + res + ")";
    }

    std::vector<Predicate> const& GetPredicates() const {
        return predicates_;
    };
};

}  // namespace dc

}  // namespace algos