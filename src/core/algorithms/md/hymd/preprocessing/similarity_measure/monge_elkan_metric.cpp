#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_metric.h"

#include <algorithm>

#include "algorithms/md/hymd/preprocessing/similarity_measure/smith_waterman_gotoh.h"

namespace algos::hymd::preprocessing::similarity_measure {
double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b) {
    return MongeElkan(a, b, [](std::string const& s1, std::string const& s2) {
        return NormalizedSmithWatermanGotoh(s1, s2);
    });
}
}  // namespace algos::hymd::preprocessing::similarity_measure
