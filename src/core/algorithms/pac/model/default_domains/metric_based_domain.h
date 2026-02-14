#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace pac::model {
/// @brief Utility base class for Domains based on IMetrizableType
class MetricBasedDomain : public IDomain {
private:
    std::vector<double> leveling_coeffs_;

    void ThrowIfEmpty() const;

protected:
    std::vector<::model::IMetrizableType const*> metrizable_types_ = {};

    /// @brief Distance between two std::byte pointers of type metrizable_types[type_num].
    /// Consider using this function instead of direct calls to metrizable_types[i]->Dist,
    /// because it takes into account @c dist_from_null_is_infiniy and @c leveling_coefficients.
    double DistBetweenBytes(std::size_t type_num, std::byte const* x, std::byte const* y) const;

    virtual double DistFromDomainInternal(Tuple const& value) const = 0;
    std::vector<std::byte const*> AllocateValues(std::vector<std::string> const&) const;

    // MetricBasedDomain's destructor is called before derived classes' destructors, so value
    // destructors have to be stored in derived classes
    std::vector<::model::Type::Destructor> GetDestructors() const;

    /// @brief Convert all needed values after types are set
    virtual void ConvertValues() {}

public:
    /// @param leveling_coefficients -- distances between individual coordinates (attributes) are
    /// multiplied by these coefficients.
    explicit MetricBasedDomain(std::vector<double>&& leveling_coefficients = {})
        : leveling_coeffs_(std::move(leveling_coefficients)) {}

    virtual void SetTypes(std::vector<::model::Type const*>&& types) override;
    virtual double DistFromDomain(Tuple const& value) const override;
};
}  // namespace pac::model
