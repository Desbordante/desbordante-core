#include "core/algorithms/pac/model/default_domains/metric_based_domain.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
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

double MetricBasedDomain::DistBetweenBytes(std::size_t type_num, std::byte const* x,
                                           std::byte const* y) const {
    if (x == nullptr || y == nullptr) {
        return dist_from_null_is_infty_ ? std::numeric_limits<double>::infinity() : 0;
    }
    return metrizable_types_[type_num]->Dist(x, y) * leveling_coeffs_[type_num];
}

std::vector<std::byte const*> MetricBasedDomain::AllocateValues(
        std::vector<std::string> const& str_values) const {
    if (str_values.size() != metrizable_types_.size()) {
        throw config::ConfigurationError(
                "Metric-based domain must be initialized with the same number of values as the "
                "number of columns used");
    }

    std::vector<std::byte const*> values(metrizable_types_.size());
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        auto const& type = metrizable_types_[i];
        auto* ptr = type->Allocate();
        type->ValueFromStr(ptr, str_values[i]);
        values[i] = ptr;
    }
    return values;
}

std::vector<Type::Destructor> MetricBasedDomain::GetDestructors() const {
    std::vector<Type::Destructor> destructors;
    std::ranges::transform(metrizable_types_, std::back_inserter(destructors),
                           std::mem_fn(&::model::Type::GetDestructor));
    return destructors;
}

void MetricBasedDomain::SetTypes(std::vector<Type const*>&& types) {
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
    std::ranges::fill_n(std::back_inserter(leveling_coeffs_),
                        types.size() - leveling_coeffs_.size(), 1);

    tuple_type_ = std::make_shared<TupleType>(std::move(types));
    ConvertValues();
}

double MetricBasedDomain::DistFromDomain(Tuple const& value) const {
    assert(value.size() == metrizable_types_.size());
    ThrowIfEmpty();

    return DistFromDomainInternal(value);
}
}  // namespace pac::model
