#pragma once

#include <memory>
#include <string>
#include <vector>

#include "pac/model/comparable_tuple_type.h"
#include "type.h"

namespace pac::model {
/// @brief An ordered domain on metric space of attribute values.
/// @note that comparer must agree with metric, i. e.
/// x in D iff exist a, b in D such that a <= x <= b
class IDomain {
protected:
    std::shared_ptr<ComparableTupleType> tuple_type_;

public:
    /// @brief Distance from Domain to @c value (i. e. inf_{a \in D} dist(@c value, a)).
    /// Must be zero for any value inside the Domain.
    virtual double DistFromDomain(Tuple const& value) const = 0;
    virtual std::string ToString() const = 0;
    virtual void SetTypes(std::vector<::model::Type const*>&&) = 0;

    ComparableTupleType const& GetTupleType() const {
        return *tuple_type_;
    }

    std::shared_ptr<ComparableTupleType> GetTupleTypePtr() const {
        return tuple_type_;
    }

    virtual ~IDomain() = default;
};
}  // namespace pac::model
