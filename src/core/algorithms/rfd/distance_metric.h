#pragma once

#include <memory>

#include "core/util/custom_metric/custom_metric.h"
#include "core/util/export.h"

namespace algos::rfd {

class DESBORDANTE_EXPORT AbsoluteThresholdMetric : public ::util::ICustomMetric {
public:
    explicit AbsoluteThresholdMetric(double tolerance) : tolerance_(tolerance) {}

    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override;

private:
    double tolerance_ = 0.0;
};

std::shared_ptr<::util::ICustomMetric> EqualityMetric();
std::shared_ptr<::util::ICustomMetric> LevenshteinMetric();
std::shared_ptr<::util::ICustomMetric> AbsoluteDifferenceMetric();
std::shared_ptr<::util::ICustomMetric> AbsoluteThresholdMetricFactory(double tolerance);

}  // namespace algos::rfd
