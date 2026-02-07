#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"

namespace pac::model {
/// @brief Ordered domain in a metric space of attribute values.
/// Despite the name, it can be a closed set.
/// @note that @c tuple_type must agree with Domain, i. e.
/// x in D iff exist a, b in D such that a <= x <= b
class IDomain {
protected:
    std::shared_ptr<TupleType> tuple_type_;
    bool dist_from_null_is_infty_ = false;

public:
    virtual ~IDomain() = default;

    /// @brief Distance from this Domain @c value.
    /// Must be zero when value is in Domain
    virtual double DistFromDomain(Tuple const& value) const = 0;
    virtual std::string ToString() const = 0;

    // Types are set by algorithm after input table is read, so they cannot be set in constructor.
    virtual void SetTypes(std::vector<::model::Type const*>&&) {}

    // Dist_from_null is algorithm parameter, so it cannot be set in constructor.
    void SetDistFromNullIsInfinity(bool value) {
        dist_from_null_is_infty_ = value;
    }

    std::shared_ptr<TupleType> GetTupleTypePtr() const {
        return tuple_type_;
    }
};
}  // namespace pac::model
