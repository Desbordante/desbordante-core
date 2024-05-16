#include <gtest/gtest.h>

#include "algorithms/md/hymd/preprocessing/similarity_measure/date_dif_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_distance.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/number_dif_similarity_measure.h"

namespace tests {

struct SimilarityTestParams {
    std::function<double(std::string, std::string)> similarityFunction;
    std::string str1;
    std::string str2;
    double expected;
};

class SimilarityMetricTest : public ::testing::TestWithParam<SimilarityTestParams> {};

TEST_P(SimilarityMetricTest, ComputesCorrectSimilarity) {
    auto params = GetParam();
    double result = params.similarityFunction(params.str1, params.str2);
    EXPECT_NEAR(result, params.expected, 0.001);
}

INSTANTIATE_TEST_SUITE_P(
        LCSTests, SimilarityMetricTest,
        ::testing::Values(
                // lcs
                SimilarityTestParams{LongestCommonSubsequence, "", "", 0.0},
                SimilarityTestParams{LongestCommonSubsequence, "hello", "", 0.0},
                SimilarityTestParams{LongestCommonSubsequence, "", "world", 0.0},
                SimilarityTestParams{LongestCommonSubsequence, "kitten", "sitting", 4.0},
                SimilarityTestParams{LongestCommonSubsequence, "abcdef", "xyabdxe", 4.0},
                SimilarityTestParams{LongestCommonSubsequence, "abcdef", "xyz", 0.0},
                SimilarityTestParams{LongestCommonSubsequence, "aaa", "aaa", 3.0},
                // jaccard
                SimilarityTestParams{JaccardIndex, "", "", 1.0},
                SimilarityTestParams{JaccardIndex, "hello", "", 0.0},
                SimilarityTestParams{JaccardIndex, "", "world", 0.0},
                SimilarityTestParams{JaccardIndex, "kitten", "sitting", 3.0 / 7.0},
                SimilarityTestParams{JaccardIndex, "abcdef", "xyabdxe", 0.5},
                SimilarityTestParams{JaccardIndex, "abcdef", "xyz", 0.0},
                SimilarityTestParams{JaccardIndex, "aaa", "aaa", 1.0},
                // levenshtein
                SimilarityTestParams{LevenshteinDistance, "", "", 0},
                SimilarityTestParams{LevenshteinDistance, "kitten", "", 6},
                SimilarityTestParams{LevenshteinDistance, "", "sitting", 7},
                SimilarityTestParams{LevenshteinDistance, "kitten", "sitting", 3},
                SimilarityTestParams{LevenshteinDistance, "abcdef", "xyabdxe", 5},
                SimilarityTestParams{LevenshteinDistance, "abcdef", "xyz", 6},
                SimilarityTestParams{LevenshteinDistance, "aaa", "aaa", 0},
                // monge-elkan
                SimilarityTestParams{MongeElkan, "", "", 1},
                SimilarityTestParams{MongeElkan, "", "xyz", 0.0},
                SimilarityTestParams{MongeElkan, "xyz", "", 0.0},
                SimilarityTestParams{MongeElkan, "abc def xyz", "def xyz abc", 1.0},
                SimilarityTestParams{MongeElkan, "hello world", "world hello!", 0.9},
                SimilarityTestParams{MongeElkan, "kitten", "sitting", 3.0 / 7.0},
                SimilarityTestParams{MongeElkan, "abcdef", "xyz", 0.0}));

}  // namespace tests