#include "core/algorithms/pac/model/default_domains/parallelepiped.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/config/exceptions.h"
#include "core/model/types/builtin.h"

namespace pac::model {
double Parallelepiped::DistFromDomainInternal(Tuple const& value) const {
    // d(x, D) = min{rho(x, y)}, where y in D, rho is Chebyshev dist:
    //   rho(x, y) = max{|x[i] - y[i]|}, i = 1, ..., n
    // For parallelepiped only points on bounds need to be considered, i. e. y in dD
    // For each dimension bounds are first_[i] and last_[i]
    // Distance from bounds on one dimension is min{|x[i] - first_[i]|, |x[i] - last_[i]|}
    // Thus, distance becomes
    //   d(x, D) = max{min{|x[i] - first_[i]|, |x[i] - last_[i]|}}, i = 1, ..., n
    double max_dist = 0;
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        double one_dim_dist = 0;
        if (tuple_type_->CompareBytes(i, value[i], first_[i]) == ::model::CompareResult::kLess) {
            one_dim_dist = DistBetweenBytes(i, value[i], first_[i]);
        } else if (tuple_type_->CompareBytes(i, last_[i], value[i]) ==
                   ::model::CompareResult::kLess) {
            one_dim_dist = DistBetweenBytes(i, value[i], last_[i]);
        }
        max_dist = std::max(max_dist, one_dim_dist);
    }
    return max_dist;
}

void Parallelepiped::ConvertValues() {
    first_ = AllocateValues(first_str_);
    last_ = AllocateValues(last_str_);
    destructors_ = GetDestructors();

    // Check that first_ <= last_
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        if (tuple_type_->CompareBytes(i, first_[i], last_[i]) == ::model::CompareResult::kGreater) {
            throw config::ConfigurationError(
                    "Lower bound of Parallelepiped must be less or equal than the upper bound.");
        }
    }
}

Parallelepiped::~Parallelepiped() {
    for (std::size_t i = 0; i < first_.size(); ++i) {
        auto const& destroy = destructors_[i];
        if (destroy) {
            destroy(first_[i]);
            destroy(last_[i]);
        } else {
            delete[] first_[i];
            delete[] last_[i];
        }
    }
}

std::string Parallelepiped::ToString() const {
    // tuple_type_->ValueToString() would be clearer, but domain isn't guaranteed to be instantiated
    // with types by here.
    std::ostringstream oss;
    oss << '[' << StringValueToString(first_str_) << ", " << StringValueToString(last_str_) << ']';
    return oss.str();
}
}  // namespace pac::model
