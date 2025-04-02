#pragma once

#include "algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class Equality : public StringSimilarityMeasure {
public:
    Equality() : StringSimilarityMeasure("equality") {}

    model::md::Similarity operator()(std::string const& left,
                                     std::string const& right) const override;
};
}  // namespace algos::md
