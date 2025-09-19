#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <sstream>
#include <utility>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/default_domains/metric_based_domain.h"
#include "imetrizable_type.h"

namespace pac::model {
/// @brief n-ary closed ball, i. e. D = {x: d(x, center) <= r}
class Ball : public MetricBasedDomain {
private:
    Tuple center_;
    std::vector<std::string> center_str_;
    double radius_;

    /// @brief Standard distance in R^n.
    /// d(X, Y) = sqrt((x[0] - y[0])^2 + (x[1] - y[1])^2 + ... + (x[n] - y[n])^2)
    double EuclideanDist(Tuple const& x, Tuple const& y) const {
        double dist = 0;
        for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
            auto coords_dist = metrizable_types_[i]->Dist(x[i], y[i]);
            dist += coords_dist * coords_dist;
        }
        return std::sqrt(dist);
    }

protected:
    /// @brief Returns @c true if dist(x, center) < dist(y, center)
    virtual bool Compare(Tuple const& x, Tuple const& y) const override {
        auto x_radius = EuclideanDist(x, center_);
        auto y_radius = EuclideanDist(y, center_);
        return x_radius < y_radius;
    }

    virtual double DistFromDomainInternal(Tuple const& value) const override {
        return std::max(0.0, EuclideanDist(value, center_) - radius_);
    }

    virtual void ConvertValues() override {
        center_ = AllocateValues(center_str_);
    }

public:
    Ball(std::vector<std::string>&& center_str, double radius)
        : center_str_(std::move(center_str)), radius_(radius) {}

    virtual ~Ball() {
        FreeValues(center_);
    }

    virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "B(" << tuple_type_->ValueToString(center_) << ", " << radius_ << ')';
        return oss.str();
    }
};
}  // namespace pac::model
