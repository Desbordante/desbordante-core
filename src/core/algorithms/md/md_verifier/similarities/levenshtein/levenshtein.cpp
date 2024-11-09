#include "core/algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"

#include "util/levenshtein_distance.h"

namespace algos::md {
class LevenshteinSimilarity : public AbstractSimilarityMeasure<std::string_view&> {
public:
    long double operator()(std::string_view& left, std::string_view& right) override {
        auto max_distance = std::max(left.size(), right.size());
        return (max_distance - util::LevenshteinDistance(left, right)) / max_distance;
    }
};
}  // namespace algos::md