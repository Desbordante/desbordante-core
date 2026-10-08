#pragma once

#include <memory>

#include "core/util/custom_metric/custom_metric.h"

namespace algos::rfd {

std::shared_ptr<::util::ICustomMetric> EqualityMetric();
std::shared_ptr<::util::ICustomMetric> LevenshteinMetric();
std::shared_ptr<::util::ICustomMetric> AbsoluteDifferenceMetric();
std::shared_ptr<::util::ICustomMetric> AbsoluteThresholdMetricFactory(double tolerance);

}  // namespace algos::rfd
