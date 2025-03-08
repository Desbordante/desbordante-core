#pragma once

#include <string_view>

#include "algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class MongeElkanSimilarity : public StringSimilarityMeasure {
public:
    MongeElkanSimilarity() : StringSimilarityMeasure("monge-elkan") {}

    long double operator()(std::string_view left, std::string_view right) const override;
};
}  // namespace algos::md