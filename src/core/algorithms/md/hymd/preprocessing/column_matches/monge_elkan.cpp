#include "algorithms/md/hymd/preprocessing/column_matches/monge_elkan.h"

#include <iosfwd>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

#include "algorithms/md/hymd/preprocessing/column_matches/smith_waterman_gotoh.h"

namespace {
std::vector<std::string> Tokenize(std::string const& text) {
    std::istringstream iss(text);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
}  // namespace

namespace algos::hymd::preprocessing::column_matches::similarity_measures {
double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b) {
    return MongeElkan(a, b, [](std::string const& s1, std::string const& s2) {
        return NormalizedSmithWatermanGotoh(s1, s2);
    });
}

double MongeElkanString(std::string const& a, std::string const& b) {
    return MongeElkan(Tokenize(a), Tokenize(b));
}
}  // namespace algos::hymd::preprocessing::column_matches::similarity_measures
