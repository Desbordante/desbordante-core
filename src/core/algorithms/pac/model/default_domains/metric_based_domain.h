#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/util/typed_value.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace pac::model {
/// @brief Utility base class for Domains based on IMetrizableType
class MetricBasedDomain : public IDomain {
private:
    std::vector<double> leveling_coeffs_;

    void ThrowIfEmpty() const;
    void ValidateTypes(std::vector<::model::Type const*> const&) override;

protected:
    using TypedTuple = std::vector<pac::util::TypedValue>;

    std::vector<::model::IMetrizableType const*> metrizable_types_;

    /// @brief Distance between two std::byte pointers of type metrizable_types[type_num].
    /// Consider using this function instead of direct calls to metrizable_types[i]->Dist,
    /// because it takes into account @c leveling_coefficients and correctly handles NULLs
    double ComponentDistance(std::size_t component_idx, std::byte const* x,
                             std::byte const* y) const;

    virtual double DistFromDomainInternal(Tuple const& value) const = 0;
    TypedTuple AllocateValues(std::vector<std::string> const&) const;

    /// @brief Convert all needed values after types are set
    virtual void ConvertValues() {}

public:
    /// @param leveling_coefficients -- distances between individual coordinates (attributes) are
    /// multiplied by these coefficients.
    explicit MetricBasedDomain(std::vector<double>&& leveling_coefficients = {})
        : leveling_coeffs_(std::move(leveling_coefficients)) {}

    virtual double DistFromDomain(Tuple const& value) const override;
};
}  // namespace pac::model
