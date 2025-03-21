#include "algorithms/md/md_verifier/similarities/jaccard/jaccard.h"

#include "algorithms/md/hymd/preprocessing/column_matches/jaccard.h"

namespace algos::md {

model::md::Similarity JaccardSimilarity::operator()(std::string const& left,
                                                    std::string const& right) const {
    using namespace algos::hymd::preprocessing::column_matches::similarity_measures;
    return StringJaccardIndex(left, right);
}
}  // namespace algos::md
