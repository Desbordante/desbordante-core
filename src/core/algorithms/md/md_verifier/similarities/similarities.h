#pragma once

#include <memory>
#include <vector>

namespace algos::md {

class SimilarityMeasure {
private:
    std::string name_;

public:
    SimilarityMeasure(std::string const& name) : name_(name) {}

    std::string const& GetName() {
        return name_;
    }

    virtual ~SimilarityMeasure() = default;
};

class NumericSimilarityMeasure : public SimilarityMeasure {
public:
    NumericSimilarityMeasure(std::string const& name) : SimilarityMeasure(name) {}

    virtual long double operator()(long double left, long double right) const = 0;
};

class StringSimilarityMeasure : public SimilarityMeasure {
public:
    StringSimilarityMeasure(std::string const& name) : SimilarityMeasure(name) {}

    virtual long double operator()(std::string_view left, std::string_view right) const = 0;
};

std::shared_ptr<NumericSimilarityMeasure> AsNumericMeasure(std::shared_ptr<SimilarityMeasure>& ptr);

std::shared_ptr<StringSimilarityMeasure> AsStringMeasure(std::shared_ptr<SimilarityMeasure>& ptr);
}  // namespace algos::md
