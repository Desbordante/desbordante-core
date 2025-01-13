#pragma once

#include "dc/FastADC/model/predicate.h"

namespace algos::fastadc {

/**
 * @brief Storage for Predicate objects.
 *
 * A Predicate represents a relational condition between two rows from potentially different
 * columns, denoted as "t.A_i op s.A_j", where t and s represent different row identifiers, A_i
 * and A_j represent columns (which may be the same or different), and 'op' is a relational operator
 * (one of <, <=, >, >=, ==, !=). This class manages a centralized repository of Predicate objects.
 *
 * Copying of this class has been deleted, since this class in only supposed to be created in
 * FastADC algorithm and passed via pointer if needed to FastADC's structures
 */
class PredicateProvider {
private:
    using ColumnMap = std::unordered_map<ColumnOperand, Predicate>;
    using OperatorMap = std::unordered_map<ColumnOperand, ColumnMap>;
    // predicates_[op][col1][col2] corresponds to the related Predicate object
    std::unordered_map<Operator, OperatorMap> predicates_;

public:
    PredicateProvider() = default;
    PredicateProvider(PredicateProvider const&) = delete;
    PredicateProvider& operator=(PredicateProvider const&) = delete;
    PredicateProvider(PredicateProvider&&) = default;
    PredicateProvider& operator=(PredicateProvider&&) = default;

    /** Create predicate object and return pointer to it or obtain it from cache */
    PredicatePtr GetPredicate(Operator const& op, ColumnOperand const& left,
                              ColumnOperand const& right) {
        auto [iter, _] = predicates_[op][left].try_emplace(right, op, left, right);
        return &iter->second;
    }

    void Clear() {
        predicates_.clear();
    }

};

}  // namespace algos::fastadc
