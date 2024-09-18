#include "algorithms/md/hymd/preprocessing/similarity_measure/smith_waterman_gotoh.h"

#include <algorithm>
#include <vector>

namespace {
float MaxOfThree(float a, float b, float c) {
    return std::max(a, std::max(b, c));
}

float MaxOfFour(float a, float b, float c, float d) {
    return MaxOfThree(a, b, std::max(c, d));
}

float SubstitutionCompare(char a, char b) {
    return a == b ? 1.0f : -2.0f;
}

float SmithWatermanGotoh(std::string const& s, std::string const& t, float gap_value = -0.5f) {
    size_t m = s.size();
    size_t n = t.size();

    std::vector<float> v0(n, 0.0f);
    std::vector<float> v1(n, 0.0f);

    float max_score = 0.0f;

    for (size_t j = 0; j < n; ++j) {
        v0[j] = std::max(0.0f, j * gap_value + SubstitutionCompare(s[0], t[j]));
        max_score = std::max(max_score, v0[j]);
    }

    for (size_t i = 1; i < m; ++i) {
        v1[0] = MaxOfThree(0.0f, v0[0] + gap_value, SubstitutionCompare(s[i], t[0]));

        max_score = std::max(max_score, v1[0]);

        for (size_t j = 1; j < n; ++j) {
            v1[j] = MaxOfFour(0.0f, v0[j] + gap_value, v1[j - 1] + gap_value,
                              v0[j - 1] + SubstitutionCompare(s[i], t[j]));
            max_score = std::max(max_score, v1[j]);
        }

        std::swap(v0, v1);
    }

    return max_score;
}

}  // namespace

namespace algos::hymd::preprocessing::similarity_measure {

float NormalizedSmithWatermanGotoh(std::string const& a, std::string const& b,
                                   float gap_value = -0.5f) {
    if (a.empty() && b.empty()) {
        return 1.0f;
    }
    if (a.empty() || b.empty()) {
        return 0.0f;
    }

    float max_distance = std::min(a.size(), b.size()) * std::max(1.0f, gap_value);
    return SmithWatermanGotoh(a, b, gap_value) / max_distance;
}

}  // namespace algos::hymd::preprocessing::similarity_measure
