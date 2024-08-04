#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_metric.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <unordered_set>

namespace algos::hymd::preprocessing::similarity_measure {

double JaccardIndex(std::string const& s1, std::string const& s2) {
    std::istringstream iss1(s1), iss2(s2);
    std::unordered_set<std::string> set1{std::istream_iterator<std::string>{iss1},
                                         std::istream_iterator<std::string>{}};
    std::unordered_set<std::string> set2{std::istream_iterator<std::string>{iss2},
                                         std::istream_iterator<std::string>{}};

    return JaccardIndex(set1, set2);
}

}  // namespace algos::hymd::preprocessing::similarity_measure
