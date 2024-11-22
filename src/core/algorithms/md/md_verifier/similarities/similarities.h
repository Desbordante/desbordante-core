#pragma once

#include <memory>
#include <vector>

#include <enum.h>

namespace algos::md {
class NumericSimilarityMeasure;
class StringSimilarityMeasure;

class SimilarityMeasure {
private:
    std::string name_;

public:
    SimilarityMeasure(std::string const& name) : name_(name) {}

    std::string const& GetName() {
        return name_;
    }

    static std::shared_ptr<NumericSimilarityMeasure> AsNumericMeasure(
            std::shared_ptr<SimilarityMeasure> ptr) {
        auto ptr_ = std::dynamic_pointer_cast<NumericSimilarityMeasure>(ptr);
        if (!ptr_.get()) {
            throw std::runtime_error("Failed to cast" + ptr->GetName() +
                                     "similarity measure to numeric similarity measure");
        }
        return ptr_;
    }

    static std::shared_ptr<StringSimilarityMeasure> AsStringMeasure(
            std::shared_ptr<SimilarityMeasure> ptr) {
        auto ptr_ = std::dynamic_pointer_cast<StringSimilarityMeasure>(ptr);
        if (!ptr_.get()) {
            throw std::runtime_error("Failed to cast" + ptr->GetName() +
                                     "similarity measure to string similarity measure");
        }
        return ptr_;
    }
};

class NumericSimilarityMeasure : public SimilarityMeasure {
public:
    NumericSimilarityMeasure(std::string const& name) : SimilarityMeasure(name) {}

    virtual long double operator()(long double left, long double right) = 0;
};

class StringSimilarityMeasure : public SimilarityMeasure {
public:
    StringSimilarityMeasure(std::string const& name) : SimilarityMeasure(name) {}

    virtual long double operator()(std::string_view left, std::string_view right) = 0;
};
}  // namespace algos::md
