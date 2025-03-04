#pragma once

#include <memory>
#include <vector>

#include <enum.h>

namespace algos::md {

BETTER_ENUM(SimilarityMeasureType, char, kStringSimilarity = 0, kNumericSimilarity);

class SimilarityMeasure {
private:
    std::string name_;

public:
    SimilarityMeasure(std::string const& name) : name_(name) {}

    std::string const& GetName() {
        return name_;
    }

    virtual SimilarityMeasureType GetType() const = 0;

    virtual ~SimilarityMeasure() = default;
};

class NumericSimilarityMeasure : public SimilarityMeasure {
public:
    NumericSimilarityMeasure(std::string const& name) : SimilarityMeasure(name) {}

    SimilarityMeasureType GetType() const {
        return SimilarityMeasureType::kNumericSimilarity;
    }

    virtual long double operator()(long double left, long double right) const = 0;
};

class StringSimilarityMeasure : public SimilarityMeasure {
public:
    StringSimilarityMeasure(std::string const& name) : SimilarityMeasure(name) {}

    SimilarityMeasureType GetType() const {
        return SimilarityMeasureType::kStringSimilarity;
    }

    virtual long double operator()(std::string_view left, std::string_view right) const = 0;
};

std::shared_ptr<NumericSimilarityMeasure> AsNumericMeasure(std::shared_ptr<SimilarityMeasure>& ptr);

std::shared_ptr<StringSimilarityMeasure> AsStringMeasure(std::shared_ptr<SimilarityMeasure>& ptr);
}  // namespace algos::md
