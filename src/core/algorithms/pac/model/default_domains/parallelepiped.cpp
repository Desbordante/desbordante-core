#include "algorithms/pac/model/default_domains/parallelepiped.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/tuple.h"

namespace pac::model {
double Parallelepiped::ChebyshevDist(Tuple const& x, Tuple const& y) const {
    double max_dist = -1;
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        max_dist = std::max(max_dist, DistBetweenBytes(i, x[i], y[i]));
    }
    return max_dist;
}

bool Parallelepiped::ProductCompare(Tuple const& x, Tuple const& y) const {
    bool all_less = true;
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        auto comp_res = tuple_type_->CompareBytes(i, x[i], y[i]);
        if (comp_res == ::model::CompareResult::kEqual) {
            all_less = false;
        }
        if (comp_res == ::model::CompareResult::kGreater) {
            return false;
        }
    }
    return all_less;
}

double Parallelepiped::DistFromDomainInternal(Tuple const& value) const {
    // This comparison is not symmetric, so using it properly is a nightmare
    if (!ProductCompare(first_, value)) {
        return ChebyshevDist(value, first_);
    }
    if (!ProductCompare(value, last_)) {
        return ChebyshevDist(value, last_);
    }
    return 0;
}

void Parallelepiped::ConvertValues() {
    first_ = AllocateValues(first_str_);
    last_ = AllocateValues(last_str_);
    destructors_ = GetDestructors();
}

bool Parallelepiped::CompareInternal(Tuple const& x, Tuple const& y) const {
    return DistFromDomainInternal(x) < DistFromDomainInternal(y);
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
