#include "algorithms/pac/model/default_domains/ball.h"

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/tuple.h"

namespace pac::model {
double Ball::EuclideanDist(Tuple const& x, Tuple const& y) const {
    double dist = 0;
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        auto coord_dist = DistBetweenBytes(i, x[i], y[i]);
        dist += coord_dist * coord_dist;
    }
    return std::sqrt(dist);
}

Ball::~Ball() {
    for (std::size_t i = 0; i < center_.size(); ++i) {
        auto const& destroy = destructors_[i];
        if (destroy) {
            destroy(center_[i]);
        } else {
            delete[] center_[i];
        }
    }
}

std::string Ball::ToString() const {
    // tuple_type_->ValueToString() would be clearer, but domain isn't guaranteed to be instantiated
    // with types by here.
    std::ostringstream oss;
    oss << "B(" << StringValueToString(center_str_) << ", " << radius_ << ')';
    return oss.str();
}
}  // namespace pac::model
