#pragma once

#include <string>
#include <utility>
#include <vector>

#include "model/table/column.h"
#include "model/table/vertical.h"
#include "predicate.h"

namespace model {

class DC {
    std::vector<model::Predicate> predicates_;

public:
    DC(std::vector<model::Predicate> predicates) : predicates_(std::move(predicates)) {};
    DC(const DC& dc) : predicates_(dc.predicates_) {};
    DC(DC&& dc) : predicates_(std::move(dc.predicates_)) {};

    DC& operator=(DC&& dc) {
        predicates_ = std::move(dc.predicates_);
        return *this;
    }

    DC() = default;

    // returns unique columns indices from each model::Predicate which satisfy the given predicate
    template <class Pred>
    std::vector<uint> GetColumnIndicesWithOperator(Pred check) {
        std::set<uint> res;
        for (Predicate const& pred : predicates_) {
            if (check(pred.GetOperator())) {
                auto leftInd = pred.GetLeftOperand().GetColumn()->GetIndex();
                auto rightInd = pred.GetRightOperand().GetColumn()->GetIndex();
                res.insert(rightInd);
                res.insert(leftInd);
            }
        }

        return std::vector(res.begin(), res.end());
    }

    template <class Pred>
    std::vector<model::Predicate> GetPredicatesWithOperator(Pred check) {
        std::vector<model::Predicate> res;
        for (Predicate const& pred : predicates_) {
            if (check(pred.GetOperator())) {
                res.push_back(pred);
            }
        }

        return res;
    }

    std::string DCToString() {
        std::string NOT = "!";
        std::string AND = " and ";

        std::string res = predicates_[0].ToString();
        for (size_t i = 1; i < predicates_.size(); i++) {
            res += AND + predicates_[i].ToString();
        }

        return NOT + "(" + res + ")";
    }

    std::vector<model::Predicate> GetPredicates() {
        return predicates_;
    };
};

}  // namespace model