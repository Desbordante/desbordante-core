#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "dc/FastADC/model/column_operand.h"
#include "dc/FastADC/model/operator.h"
#include "model/types/bitset.h"
#include "table/typed_column_data.h"

namespace model {
class TypedColumnData;
}  // namespace model

namespace algos::fastadc {

class Predicate;

using PredicatePtr = Predicate const*;
using PredicatesVector = std::vector<PredicatePtr>;
using PredicatesSpan = std::span<PredicatePtr const>;

class PredicateProvider;

/*
 * TODO: Java code uses LongBitSet, which is like boost::dynamic_bitset, but
 * restructs number of bits in the clue to kPredicateBits. Need to investigate further whether
 * the Java's algorithm could work with predicate space more than kPredicateBits.
 * But for now we use kPredicateBits as maxumum amount of predicates
 */
constexpr auto kPredicateBits = 128;
using PredicateBitset = model::Bitset<kPredicateBits>;

/**
 * @brief Represents a predicate for FastADC.
 *
 * FastADC processes Denial Constraints (DCs) that involve comparisons between
 * pairs of rows within a dataset. A typical DC example, derived from a Functional
 * Dependency such as A -> B, is expressed as: `forall t, s in r, not (t.A = s.A and t.B != s.B)`
 * This denotes that for any pair of rows in the relation, it should not be the case
 * that while the values in column "A" are equal, the values in column "B" are unequal.
 *
 * A predicate in this context (e.g., t.A == s.A) comprises three elements to be fully
 * represented: the column operand from the first tuple ("t.A"), the comparison operator
 * ("="), and the column operand from the second tuple ("s.A").
 */

// TODO: remove code duplication cause we already have "dc/model/predicate.h" that is used for
// DC verification.
class Predicate {
private:
    Operator op_;
    ColumnOperand l_;
    ColumnOperand r_;

    mutable PredicatePtr symmetric_{};
    mutable PredicatePtr inv_TS_{};
    mutable PredicatePtr inverse_{};
    mutable PredicatesVector implications_;

public:
    Predicate(Operator const& op, ColumnOperand const& l, ColumnOperand const& r)
        : op_(op), l_(l), r_(r) {}

    // FIXME: mb pass some table representation other than vector of columns data?
    bool Satisfies(std::vector<model::TypedColumnData>& col_data, size_t t, size_t s) const;

    PredicatePtr GetSymmetric(PredicateProvider* provider) const;

    PredicatePtr GetInvTS(PredicateProvider* provider) const;

    PredicatePtr GetInverse(PredicateProvider* provider) const;

    PredicatesVector const& GetImplications(PredicateProvider* provider) const;

    Operator GetOperator() const {
        return op_;
    }

    ColumnOperand GetLeftOperand() const {
        return l_;
    }

    ColumnOperand GetRightOperand() const {
        return r_;
    }

    bool IsCrossColumn() const {
        return l_.GetColumn() != r_.GetColumn();
    }

    bool HasSameOperandsAs(Predicate const& rhs) const {
        return l_ == rhs.GetLeftOperand() && r_ == rhs.GetRightOperand();
    }

    bool HasSameOperandsAs(PredicatePtr rhs) const {
        return l_ == rhs->GetLeftOperand() && r_ == rhs->GetRightOperand();
    }

    bool operator==(Predicate const& rhs) const {
        return op_ == rhs.op_ && l_ == rhs.l_ && r_ == rhs.r_;
    }

    std::string ToString() const {
        return l_.ToString() + " " + op_.ToString() + " " + r_.ToString();
    }
};

PredicatePtr GetPredicateByType(PredicatesSpan predicates, OperatorType type);

}  // namespace algos::fastadc

template <>
struct std::hash<algos::fastadc::Predicate> {
    size_t operator()(algos::fastadc::Predicate const& k) const noexcept {
        std::size_t seed = 0;
        boost::hash_combine(seed, k.GetOperator());
        boost::hash_combine(seed, k.GetLeftOperand());
        boost::hash_combine(seed, k.GetRightOperand());
        return seed;
    }
};
