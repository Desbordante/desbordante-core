#pragma once

#include <sstream>
#include <string>

#include "predicate_set.h"

namespace algos::fastadc {

// TODO: remove code duplication cause we already have "dc/model/dc.h" that is used for
// DC verification.
class DenialConstraint {
private:
    PredicateSet predicate_set_;

public:
    explicit DenialConstraint(PredicateSet const& predicateSet) : predicate_set_(predicateSet) {}

    DenialConstraint(boost::dynamic_bitset<> const& predicates,
                     PredicateIndexProvider* predicate_index_provider)
        : predicate_set_(predicates, predicate_index_provider) {}

    DenialConstraint GetInvT1T2DC(PredicateProvider* predicate_provider) const {
        return DenialConstraint(predicate_set_.GetInvTS(predicate_provider));
    }

    PredicateSet const& GetPredicateSet() const {
        return predicate_set_;
    }

    size_t GetPredicateCount() const {
        return predicate_set_.Size();
    }

    std::string ToString() const {
        std::string const c_not = "\u00AC";
        std::string const c_and = " ∧ ";
        std::ostringstream sb;
        sb << c_not << "{ ";
        bool first = true;
        for (PredicatePtr predicate : predicate_set_) {
            if (!first) {
                sb << c_and;
            }
            sb << predicate->ToString();
            first = false;
        }
        sb << " }";
        return sb.str();
    }

    bool operator==(DenialConstraint const& other) const = default;
    bool operator!=(DenialConstraint const& other) const = default;
};

}  // namespace algos::fastadc

template <>
struct std::hash<algos::fastadc::DenialConstraint> {
    size_t operator()(algos::fastadc::DenialConstraint const& k) const noexcept {
        return k.GetPredicateSet().Hash();
    }
};
