#pragma once

#include <functional>
#include <memory>
#include <string>

#include "core/util/export.h"

namespace algos::rfd {

class DESBORDANTE_EXPORT SimilarityMetric {
public:
    virtual ~SimilarityMetric() = default;
    virtual double Compare(std::string const& a, std::string const& b) const = 0;
};

class FunctionSimilarityMetric : public SimilarityMetric {
public:
    using Func = std::function<double(std::string const&, std::string const&)>;
    explicit FunctionSimilarityMetric(Func func);
    double Compare(std::string const& a, std::string const& b) const override;

private:
    Func func_;
};

std::shared_ptr<SimilarityMetric> LevenshteinMetric();
std::shared_ptr<SimilarityMetric> EqualityMetric();
std::shared_ptr<SimilarityMetric> AbsoluteDifferenceMetric();
std::shared_ptr<SimilarityMetric> AbsoluteThresholdMetric(double diff);

}  // namespace algos::rfd
