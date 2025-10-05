#pragma once

#include <memory>
#include <sstream>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "dc/FastADC/model/predicate.h"
#include "dc/FastADC/model/predicate_set.h"

namespace algos::fastadc {

// TODO: remove code duplication cause we already have "dc/model/dc.h" that is used for
// DC verification.
class DenialConstraint {
private:
    PredicateSet predicate_set_;
    std::shared_ptr<RelationalSchema const> schema_;

public:
    explicit DenialConstraint(PredicateSet const& predicateSet,
                              std::shared_ptr<RelationalSchema const> schema)
        : predicate_set_(predicateSet), schema_(schema) {}

    DenialConstraint(boost::dynamic_bitset<> const& predicates,
                     std::shared_ptr<PredicateIndexProvider> predicate_index_provider,
                     std::shared_ptr<RelationalSchema const> schema)
        : predicate_set_(predicates, predicate_index_provider), schema_(schema) {}

    DenialConstraint GetInvT1T2DC(PredicateProvider* predicate_provider) const {
        return DenialConstraint(predicate_set_.GetInvTS(predicate_provider), schema_);
    }

    PredicateSet const& GetPredicateSet() const noexcept {
        return predicate_set_;
    }

    size_t GetPredicateCount() const noexcept {
        return predicate_set_.Size();
    }

    std::shared_ptr<RelationalSchema const> GetSchema() const {
        return schema_;
    }

    std::string ToString() const noexcept {
        static std::string const kCNot = "\u00AC";
        static std::string const kCAnd = " ∧ ";
        std::ostringstream sb;
        sb << kCNot << "( ";
        std::string separator;
        for (PredicatePtr predicate : predicate_set_) {
            sb << separator << predicate->ToString();
            separator = kCAnd;
        }
        sb << " )";
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
