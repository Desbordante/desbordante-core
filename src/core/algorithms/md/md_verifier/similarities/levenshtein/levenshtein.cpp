#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"

#include "util/levenshtein_distance.h"

namespace algos::md {

model::md::Similarity LevenshteinSimilarity::operator()(std::string const& left,
                                                        std::string const& right) const {
    long double max_distance = std::max(left.size(), right.size());
    return (max_distance - util::LevenshteinDistance(left, right)) / max_distance;
}
}  // namespace algos::md
