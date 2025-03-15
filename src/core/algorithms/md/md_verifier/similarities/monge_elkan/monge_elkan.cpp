#include "algorithms/md/hymd/preprocessing/column_matches/monge_elkan.h"

#include "algorithms/md/md_verifier/similarities/monge_elkan/monge_elkan.h"

namespace algos::md {

long double MongeElkanSimilarity::operator()(std::string_view left, std::string_view right) const {
    using namespace algos::hymd::preprocessing::column_matches::similarity_measures;
    return MongeElkanString(std::string(left), std::string(right));
}
}  // namespace algos::md