#pragma once

#include <string>
#include <vector>
#include <utility>

#include "model/table/column.h"
#include "model/table/vertical.h"
#include "predicate.h"

namespace model {

class DC {
    std::vector<model::Predicate> predicates_;

public:
    DC(std::vector<model::Predicate> predicates) : predicates_(std::move(predicates)){};
    DC(const DC& dc) : predicates_(dc.predicates_){};
    DC(DC&& dc) : predicates_(std::move(dc.predicates_)){};

    DC& operator=(DC&& dc) {
        predicates_ = std::move(dc.predicates_);
        return *this;
    }

    DC() = default;

    std::vector<unsigned> GetColumnIndicesWithOperator(Operator op);
    std::string DCToString();

    std::vector<model::Predicate> GetPredicates() {
        return predicates_;
    };
};

}  // namespace model