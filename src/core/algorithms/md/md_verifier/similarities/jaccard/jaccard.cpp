#include "algorithms/md/md_verifier/similarities/jaccard/jaccard.h"

#include "algorithms/md/hymd/preprocessing/column_matches/jaccard.h"

namespace algos::md {

long double JaccardSimilarity::operator()(std::string_view left, std::string_view right) const {
    using namespace algos::hymd::preprocessing::column_matches::similarity_measures;
    return StringJaccardIndex(std::string(left), std::string(right));
}
}  // namespace algos::md