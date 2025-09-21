#include "algorithms/pac/model/default_domains/parallelepiped.h"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/tuple.h"
#include "builtin.h"

namespace pac::model {
double Parallelepiped::ChebyshevDist(Tuple const& x, Tuple const& y) const {
    double max_dist = -1;
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        max_dist = std::max(max_dist, DistBetweenBytes(i, x[i], y[i]));
    }
    return max_dist;
}

double Parallelepiped::DistFromDomainInternal(Tuple const& value) const {
    if (tuple_type_->Less(value, first_)) {
        return ChebyshevDist(value, first_);
    }
    if (tuple_type_->Less(last_, value)) {
        return ChebyshevDist(value, last_);
    }
    return 0;
}

void Parallelepiped::ConvertValues() {
    first_ = AllocateValues(first_str_);
    last_ = AllocateValues(last_str_);
    destructors_ = GetDestructors();
}

bool Parallelepiped::CompareInternal([[maybe_unused]] Tuple const& x,
                                     [[maybe_unused]] Tuple const& y) const {
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        auto comp_res = tuple_type_->CompareBytes(i, x[i], y[i]);
        if (comp_res == ::model::CompareResult::kLess) {
            return true;
        }
        if (comp_res == ::model::CompareResult::kGreater) {
            return false;
        }
    }
    return false;
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
