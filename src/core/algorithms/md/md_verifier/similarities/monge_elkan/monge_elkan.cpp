#include "algorithms/md/hymd/preprocessing/column_matches/monge_elkan.h"

#include "algorithms/md/md_verifier/similarities/monge_elkan/monge_elkan.h"

namespace algos::md {

model::md::Similarity MongeElkanSimilarity::operator()(std::string const& left,
                                                       std::string const& right) const {
    using namespace algos::hymd::preprocessing::column_matches::similarity_measures;
    return MongeElkanString(left, right);
}
}  // namespace algos::md
