#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"

#include "util/levenshtein_distance.h"

namespace algos::md {

long double LevenshteinSimilarity::operator()(std::string_view left, std::string_view right) const {
    long double max_distance = std::max(left.size(), right.size());
    return (max_distance - util::LevenshteinDistance(left, right)) / max_distance;
}
}  // namespace algos::md