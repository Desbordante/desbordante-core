#include "algorithms/md/md_verifier/similarities/equality/equality.h"

#include "util/levenshtein_distance.h"

namespace algos::md {

model::md::Similarity Equality::operator()(std::string const& left,
                                           std::string const& right) const {
    return left == right ? 1.0 : 0.0;
}
}  // namespace algos::md
