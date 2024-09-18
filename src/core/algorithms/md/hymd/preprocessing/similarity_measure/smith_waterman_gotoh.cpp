#include "algorithms/md/hymd/preprocessing/similarity_measure/smith_waterman_gotoh.h"

#include <algorithm>
#include <vector>

namespace {
double SubstitutionCompare(char a, char b) {
    return a == b ? 1.0 : -2.0;
}

double SmithWatermanGotoh(std::string const& s, std::string const& t, double gap_value = -0.5) {
    size_t m = s.size();
    size_t n = t.size();

    std::vector<double> v0(n, 0.0);
    std::vector<double> v1(n, 0.0);

    double max_score = 0.0;

    for (size_t j = 0; j < n; ++j) {
        v0[j] = std::max(0.0, j * gap_value + SubstitutionCompare(s[0], t[j]));
        max_score = std::max(max_score, v0[j]);
    }

    for (size_t i = 1; i < m; ++i) {
        v1[0] = std::max({0.0, v0[0] + gap_value, SubstitutionCompare(s[i], t[0])});

        max_score = std::max(max_score, v1[0]);

        for (size_t j = 1; j < n; ++j) {
            v1[j] = std::max({0.0, v0[j] + gap_value, v1[j - 1] + gap_value,
                              v0[j - 1] + SubstitutionCompare(s[i], t[j])});
            max_score = std::max(max_score, v1[j]);
        }

        std::swap(v0, v1);
    }

    return max_score;
}

}  // namespace

namespace algos::hymd::preprocessing::similarity_measure {

double NormalizedSmithWatermanGotoh(std::string const& a, std::string const& b, double gap_value) {
    if (a.empty() && b.empty()) {
        return 1.0f;
    }
    if (a.empty() || b.empty()) {
        return 0.0f;
    }

    double max_distance = std::min(a.size(), b.size()) * std::max(1.0, gap_value);
    return SmithWatermanGotoh(a, b, gap_value) / max_distance;
}

}  // namespace algos::hymd::preprocessing::similarity_measure
