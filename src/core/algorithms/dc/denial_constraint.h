#pragma once

#include <sstream>
#include <string>

#include "predicate_set.h"

namespace model {

class DenialConstraint {
private:
    PredicateSet predicate_set_;

public:
    explicit DenialConstraint(PredicateSet const& predicateSet) : predicate_set_(predicateSet) {}

    DenialConstraint(boost::dynamic_bitset<> const& predicates) : predicate_set_(predicates) {}

    DenialConstraint GetInvT1T2DC() const {
        return DenialConstraint(predicate_set_.GetInvTS());
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
};

}  // namespace model

namespace std {
template <>
struct hash<model::DenialConstraint> {
    size_t operator()(model::DenialConstraint const& k) const noexcept {
        return k.GetPredicateSet().Hash();
    }
};
}  // namespace std
