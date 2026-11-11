#include "core/algorithms/pac/model/default_domains/metric_based_domain.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"

namespace pac::model {
using namespace ::model;

void MetricBasedDomain::ThrowIfEmpty() const {
    if (metrizable_types_.empty()) {
        throw std::runtime_error(
                "Metric-based domain must be instantiated with types (SetTypes) before use");
    }
}

void MetricBasedDomain::ValidateTypes(std::vector<Type const*> const& types) {
    metrizable_types_ = std::vector<IMetrizableType const*>(types.size());
    for (std::size_t i = 0; i < types.size(); ++i) {
        auto const* metrizable_type = dynamic_cast<IMetrizableType const*>(types[i]);
        if (!metrizable_type) {
            throw config::ConfigurationError("Cannot use metric-based domain, because column #" +
                                             std::to_string(i) + " has type " +
                                             types[i]->ToString() + ", which is not metrizable");
        }
        metrizable_types_[i] = metrizable_type;
    }

    // All leveling coefficients that are not specified are 1
    leveling_coeffs_.insert(leveling_coeffs_.end(), types.size() - leveling_coeffs_.size(), 1);
}

double MetricBasedDomain::ComponentDistance(std::size_t component_idx, std::byte const* x,
                                            std::byte const* y) const {
    if (x == nullptr || y == nullptr) {
        return 0;
    }
    return metrizable_types_[component_idx]->Dist(x, y) * leveling_coeffs_[component_idx];
}

MetricBasedDomain::TypedTuple MetricBasedDomain::AllocateValues(
        std::vector<std::string> const& str_values) const {
    if (str_values.size() != metrizable_types_.size()) {
        throw config::ConfigurationError(
                "Metric-based domain must be initialized with the same number of values as the "
                "number of columns used");
    }

    TypedTuple values;
    values.reserve(metrizable_types_.size());
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        values.emplace_back(str_values[i], metrizable_types_[i]);
    }
    return values;
}

double MetricBasedDomain::DistFromDomain(Tuple const& value) const {
    assert(value.size() == metrizable_types_.size());
    ThrowIfEmpty();

    return DistFromDomainInternal(value);
}
}  // namespace pac::model
