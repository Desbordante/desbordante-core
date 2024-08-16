#include <gtest/gtest.h>

#include "algorithms/md/hymd/preprocessing/similarity_measure/date_dif_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_distance.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/number_dif_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/smith_waterman_gotoh.h"

using namespace algos::hymd::preprocessing::similarity_measure;

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
        Default, SimilarityMetricTest,
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
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); }, "",
                        "", 1.0},
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); },
                        "hello", "", 0.0},
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); }, "",
                        "world", 0.0},
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); },
                        "abc cde", "abc", 0.5},
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); },
                        "abc cde", "abc def", 1.0 / 3.0},
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); },
                        "word1", "word2", 0},
                SimilarityTestParams{
                        [](std::string a, std::string b) { return StringJaccardIndex(a, b); },
                        "word", "word", 1.0},
                // levenshtein
                SimilarityTestParams{LevenshteinDistance, "", "", 0},
                SimilarityTestParams{LevenshteinDistance, "kitten", "", 6},
                SimilarityTestParams{LevenshteinDistance, "", "sitting", 7},
                SimilarityTestParams{LevenshteinDistance, "kitten", "sitting", 3},
                SimilarityTestParams{LevenshteinDistance, "abcdef", "xyabdxe", 5},
                SimilarityTestParams{LevenshteinDistance, "abcdef", "xyz", 6},
                SimilarityTestParams{LevenshteinDistance, "aaa", "aaa", 0}));

struct MongeElkanTestParams {
    std::vector<std::string> vec1;
    std::vector<std::string> vec2;
    double expected;
};

class MongeElkanMetricTest : public ::testing::TestWithParam<MongeElkanTestParams> {};

TEST_P(MongeElkanMetricTest, ComputesCorrectSimilarity) {
    auto params = GetParam();
    double result = MongeElkan(params.vec1, params.vec2);
    EXPECT_NEAR(result, params.expected, 0.001);
}

INSTANTIATE_TEST_SUITE_P(
        Default, MongeElkanMetricTest,
        ::testing::Values(MongeElkanTestParams{{}, {}, 1.0}, MongeElkanTestParams{{"abc"}, {}, 0.0},
                          MongeElkanTestParams{{}, {"abc"}, 0.0},
                          MongeElkanTestParams{{"abc", "def", "xyz"}, {"def", "xyz", "abc"}, 1.0},
                          MongeElkanTestParams{{"hello", "word"}, {"world", "helo"}, 7.0 / 8.0},
                          MongeElkanTestParams{{"abc"}, {"xyz"}, 0.0},
                          MongeElkanTestParams{{"abc", "def"}, {"abc"}, std::sqrt(0.5)},
                          MongeElkanTestParams{{"abc"}, {"abc", "def"}, std::sqrt(0.5)},
                          MongeElkanTestParams{{"abc"}, {"abc", "abc"}, 1.0},
                          MongeElkanTestParams{{"word1", "word2"}, {"Word2", "Word1"}, 4.0 / 5.0}));

}  // namespace tests
