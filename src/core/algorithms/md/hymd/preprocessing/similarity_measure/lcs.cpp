#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs.h"

#include <algorithm>
#include <vector>

namespace algos::hymd::preprocessing::similarity_measure {

int Lcs(std::string const& left, std::string const& right) {
    int const n = left.size();
    int const m = right.size();

    std::vector<int> v0(m + 1, 0);
    std::vector<int> v1(m + 1, 0);

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (left[i - 1] == right[j - 1]) {
                v1[j] = v0[j - 1] + 1;
            } else {
                v1[j] = std::max(v1[j - 1], v0[j]);
            }
        }
        std::swap(v0, v1);
    }

    return v0[m];
}

float LongestCommonSubsequence(std::string const& left, std::string const& right) {
    if (left.empty() && right.empty()) {
        return 1.0f;
    }

    if (left.empty() || right.empty()) {
        return 0.0f;
    }

    return static_cast<float>(Lcs(left, right)) / std::max(left.size(), right.size());
}
}  // namespace algos::hymd::preprocessing::similarity_measure
