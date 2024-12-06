#include <string_view>

#include "algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class LevenshteinSimilarity : public StringSimilarityMeasure {
public:
    LevenshteinSimilarity() : StringSimilarityMeasure("levenshtein") {}
    long double operator()(std::string_view left, std::string_view right) const override;
};
}  // namespace algos::md