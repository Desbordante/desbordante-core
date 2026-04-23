#pragma once

#include <functional>
#include <memory>
#include <string>

namespace algos::rfd {

class SimilarityMetric {
public:
    virtual ~SimilarityMetric() = default;
    virtual double Compare(const std::string& a, const std::string& b) const = 0;
};

class FunctionSimilarityMetric : public SimilarityMetric {
public:
    using Func = std::function<double(const std::string&, const std::string&)>;
    explicit FunctionSimilarityMetric(Func func);
    double Compare(const std::string& a, const std::string& b) const override;

private:
    Func func_;
};

std::shared_ptr<SimilarityMetric> LevenshteinMetric();
std::shared_ptr<SimilarityMetric> EqualityMetric();

} // namespace algos::rfd
