#include "metric_based_domain.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/tuple.h"
#include "exceptions.h"

namespace pac::model {
using namespace ::model;

void MetricBasedDomain::ThrowIfEmpty() const {
    if (metrizable_types_.empty()) {
        throw std::runtime_error(
                "Metric-based domain must be instantiated with types (SetTypes) before use");
    }
}

bool MetricBasedDomain::Compare(Tuple const& x, Tuple const& y) const {
    ThrowIfEmpty();
    assert(x.size() >= metrizable_types_.size());
    assert(y.size() >= metrizable_types_.size());

    return CompareInternal(x, y);
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
    assert(str_values.size() >= metrizable_types_.size());

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
    using namespace std::placeholders;

    metrizable_types_ = {};
    std::ranges::transform(types, std::back_inserter(metrizable_types_), [](Type const* type) {
        auto const* metrizable_type = dynamic_cast<IMetrizableType const*>(type);
        if (!metrizable_type) {
            throw config::ConfigurationError(
                    "To use metric-based domain, all affected columns must have "
                    "metrizable types");
        }
        return metrizable_type;
    });

    // All leveling coefficients that are not specified are 1
    std::ranges::fill_n(std::back_inserter(leveling_coeffs_),
                        types.size() - leveling_coeffs_.size(), 1);

    tuple_type_ = std::make_shared<ComparableTupleType>(
            std::move(types), std::bind(&MetricBasedDomain::Compare, this, _1, _2));
    ConvertValues();
}

double MetricBasedDomain::DistFromDomain(Tuple const& value) const {
    assert(value.size() >= metrizable_types_.size());
    ThrowIfEmpty();

    return DistFromDomainInternal(value);
}
}  // namespace pac::model
