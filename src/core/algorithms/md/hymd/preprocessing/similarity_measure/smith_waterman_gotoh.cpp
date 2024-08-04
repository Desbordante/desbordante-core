#include "algorithms/md/hymd/preprocessing/similarity_measure/smith_waterman_gotoh.h"

#include <algorithm>
#include <vector>

namespace {
double SubstitutionCompare(char a, char b) {
    return a == b ? 1.0 : -2.0;
}

double SmithWatermanGotoh(std::string const& s, std::string const& t, double gapValue = -0.5) {
    std::vector<double> v0(t.size() + 1, 0.0);
    std::vector<double> v1(t.size() + 1, 0.0);

    double max = 0.0;

    for (size_t i = 1; i <= s.size(); ++i) {
        for (size_t j = 1; j <= t.size(); ++j) {
            double match = v0[j - 1] + SubstitutionCompare(s[i - 1], t[j - 1]);
            double delete_from_s = v0[j] + gapValue;
            double delete_from_t = v1[j - 1] + gapValue;
            v1[j] = std::max({0.0, match, delete_from_s, delete_from_t});
            max = std::max(max, v1[j]);
        }
        std::swap(v0, v1);
    }

    return max;
}
}  // namespace

namespace algos::hymd::preprocessing::similarity_measure {
double NormalizedSmithWatermanGotoh(std::string const& s, std::string const& t, double gapValue) {
    if (s.empty() && t.empty()) return 1.0;
    if (s.empty() || t.empty()) return 0.0;

    double max_distance = std::min(s.size(), t.size()) * std::max(1.0, gapValue);
    return SmithWatermanGotoh(s, t, gapValue) / max_distance;
}
}  // namespace algos::hymd::preprocessing::similarity_measure
