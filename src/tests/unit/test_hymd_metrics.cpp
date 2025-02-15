#include <gtest/gtest.h>

#include "algorithms/md/hymd/preprocessing/column_matches/date_difference.h"
#include "algorithms/md/hymd/preprocessing/column_matches/jaccard.h"
#include "algorithms/md/hymd/preprocessing/column_matches/lcs.h"
#include "algorithms/md/hymd/preprocessing/column_matches/levenshtein.h"
#include "algorithms/md/hymd/preprocessing/column_matches/monge_elkan.h"
#include "algorithms/md/hymd/preprocessing/column_matches/number_difference.h"
#include "algorithms/md/hymd/preprocessing/column_matches/smith_waterman_gotoh.h"

using namespace algos::hymd::preprocessing::column_matches;

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
                SimilarityTestParams{similarity_measures::Lcs, "", "", 0},
                SimilarityTestParams{similarity_measures::Lcs, "hello", "", 0},
                SimilarityTestParams{similarity_measures::Lcs, "", "world", 0},
                SimilarityTestParams{similarity_measures::Lcs, "kitten", "sitting", 4},
                SimilarityTestParams{similarity_measures::Lcs, "abcdef", "xyabdxe", 4},
                SimilarityTestParams{similarity_measures::Lcs, "abcdef", "xyz", 0},
                SimilarityTestParams{similarity_measures::Lcs, "aaa", "aaa", 3},
                // jaccard
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "", "", 1.0},
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "hello", "", 0.0},
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "", "world", 0.0},
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "abc cde", "abc", 0.5},
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "abc cde", "abc def", 1.0 / 3.0},
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "word1", "word2", 0},
                SimilarityTestParams{[](std::string a, std::string b) {
                                         return similarity_measures::StringJaccardIndex(a, b);
                                     },
                                     "word", "word", 1.0}));

struct MongeElkanTestParams {
    std::vector<std::string> vec1;
    std::vector<std::string> vec2;
    double expected;
};

class MongeElkanMetricTest : public ::testing::TestWithParam<MongeElkanTestParams> {};

TEST_P(MongeElkanMetricTest, ComputesCorrectSimilarity) {
    auto params = GetParam();
    double result = similarity_measures::MongeElkan(params.vec1, params.vec2);
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
