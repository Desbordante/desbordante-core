#include <vector>
#include <string>
#include <utility>

#include "operator.h"
#include "data_frame.h"
#include "schema_value.h"
#include "single_attribute_predicate.h"

using namespace algos::fastod;

std::vector<std::vector<SingleAttributePredicate>> SingleAttributePredicate::cache_;

SingleAttributePredicate::SingleAttributePredicate(int attribute, Operator const& op) noexcept
    : attribute_(attribute), operator_(std::move(op)) { }

int SingleAttributePredicate::GetAttribute() const noexcept {
    return attribute_;
}

Operator const& SingleAttributePredicate::GetOperator() const noexcept {
    return operator_;
}

std::string SingleAttributePredicate::ToString() const {
    return std::to_string(attribute_ + 1) + operator_.ToString();
}

int SingleAttributePredicate::GetHashCode() const noexcept {
    return attribute_;
}

bool SingleAttributePredicate::Violate(DataFrame const& data,
                                       int first_tuple_index,
                                       int second_tuple_index) const noexcept {
    SchemaValue const& first_value = data.GetValue(first_tuple_index, attribute_);
    SchemaValue const& second_value = data.GetValue(second_tuple_index, attribute_);
    
    return operator_.Violate(first_value, second_value);
}

SingleAttributePredicate SingleAttributePredicate::GetInstance(std::size_t attribute, Operator const& op) {
    while (attribute >= cache_.size()) {
        std::vector<SingleAttributePredicate> predicates;

        for (Operator const& op : Operator::SupportedOperators()) {
            predicates.push_back(SingleAttributePredicate(cache_.size(), op));
        }

        cache_.push_back(predicates);
    }

    std::vector<SingleAttributePredicate> const& predicates = cache_[attribute];
    int predicate_index = op.GetTypeAsInt();
    
    return predicates[predicate_index];
}

namespace algos::fastod {

bool operator==(SingleAttributePredicate const& x, SingleAttributePredicate const& y) {
    return x.GetAttribute() == y.GetAttribute() && x.GetOperator() == y.GetOperator();
}

} // namespace algos::fastod

