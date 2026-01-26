#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/pac/model/tuple.h"
#include "pac/model/default_domains/metric_based_domain.h"

namespace pac::model {
/// @brief Closed ball, defined by @c center and @c radius.
/// D = {x | d(x, @c center) <= @c radius}
class Ball final : public MetricBasedDomain {
private:
    Tuple center_;
    std::vector<std::string> center_str_;
    double radius_;
    std::vector<::model::Type::Destructor> destructors_;

    /// @brief Euclidean distance.
    /// d(X, Y) = sqrt((x[0] - y[0])^2 + (x[1] - y[1])^2 + ... + (x[n] - y[n])^2)
    double EuclideanDist(Tuple const& x, Tuple const& y) const;

protected:
    virtual double DistFromDomainInternal(Tuple const& value) const override {
        return std::max(0.0, EuclideanDist(value, center_) - radius_);
    }

    virtual void ConvertValues() override {
        center_ = AllocateValues(center_str_);
        destructors_ = GetDestructors();
    }

    /// @brief Compare radii.
    /// X < Y iff d(X, @c center) < d(Y, @c center)
    virtual bool CompareInternal(Tuple const& x, Tuple const& y) const override {
        return EuclideanDist(x, center_) < EuclideanDist(y, center_);
    }

public:
    Ball(std::vector<std::string>&& center_str, double radius,
         std::vector<double>&& leveling_coefficients = {})
        : MetricBasedDomain(std::move(leveling_coefficients)),
          center_str_(std::move(center_str)),
          radius_(radius) {}

    virtual ~Ball();
    virtual std::string ToString() const override;
};
}  // namespace pac::model
