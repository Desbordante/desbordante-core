#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/model/types/type.h"
#include "core/util/export.h"

namespace pac::model {
/// @brief Ordered domain in a metric space of attribute values.
/// Despite the name, it can be a closed set.
/// @note that @c tuple_type must agree with Domain, i. e.
/// x in D iff exist a, b in D such that a <= x <= b
class DESBORDANTE_EXPORT IDomain {
protected:
    std::shared_ptr<TupleType> tuple_type_;

    // Validate types if needed, and do all other types-related business
    virtual void ValidateTypes(std::vector<::model::Type const*> const&) {}

    // Called right after tuple_type_ is created
    virtual void AfterSetTypes() {}

public:
    virtual ~IDomain() = default;

    /// @brief Distance from this Domain @c value.
    /// Must be zero when value is in Domain
    virtual double DistFromDomain(Tuple const& value) const = 0;
    virtual std::string ToString() const = 0;

    // Types are set by algorithm after input table is read, so they cannot be set in constructor.
    void SetTypes(std::vector<::model::Type const*>&& types) {
        ValidateTypes(types);
        tuple_type_ = std::make_shared<TupleType>(std::move(types));
        AfterSetTypes();
    }

    std::shared_ptr<TupleType> GetTupleTypePtr() const {
        return tuple_type_;
    }
};
}  // namespace pac::model
