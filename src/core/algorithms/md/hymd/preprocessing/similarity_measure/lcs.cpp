#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs.h"

#include <vector>

double LongestCommonSubsequence(std::string const& word1, std::string const& word2) {
    size_t m = word1.size();
    size_t n = word2.size();

    std::vector<std::vector<size_t>> lcs_table(m + 1, std::vector<size_t>(n + 1, 0));

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (word1[i - 1] == word2[j - 1]) {
                lcs_table[i][j] = lcs_table[i - 1][j - 1] + 1;
            } else {
                lcs_table[i][j] = std::max(lcs_table[i - 1][j], lcs_table[i][j - 1]);
            }
        }
    }

    return lcs_table[m][n];
}