#pragma once

#include "algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class LevenshteinSimilarity : public StringSimilarityMeasure {
public:
    LevenshteinSimilarity() : StringSimilarityMeasure("levenshtein") {}

    model::md::Similarity operator()(std::string const& left,
                                     std::string const& right) const override;
};
}  // namespace algos::md
