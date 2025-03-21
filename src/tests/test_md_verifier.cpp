#include <limits>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier.h"
#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
using namespace config::names;
using MDVerifier = algos::md::MDVerifier;
using DecisionBoundary = model::md::DecisionBoundary;
using SimilarityMeasure = algos::md::SimilarityMeasure;

struct MDVerifierParams {
    algos::StdParamsMap params;
    bool const expected;
    DecisionBoundary suggestions;

    MDVerifierParams(CSVConfig const& csv_config,
                     std::vector<algos::md::MDVerifierColumnSimilarityClassifier> lhs,
                     algos::md::MDVerifierColumnSimilarityClassifier rhs, bool const expected,
                     DecisionBoundary suggestions)
        : expected(expected), suggestions(suggestions) {
        config::InputTable table = std::make_unique<CSVParser>(csv_config);
        params = {{kLeftTable, table}, {kMDLHS, lhs}, {kMDRHS, rhs}};
    }
};

struct MDVerifierHighlightsParams {
    algos::StdParamsMap params;
    bool const expected;
    std::vector<std::string> highlights;

    MDVerifierHighlightsParams(CSVConfig const& csv_config,
                               std::vector<algos::md::MDVerifierColumnSimilarityClassifier> lhs,
                               algos::md::MDVerifierColumnSimilarityClassifier rhs,
                               bool const expected, std::vector<std::string> highlights)
        : expected(expected), highlights(std::move(highlights)) {
        config::InputTable table = std::make_unique<CSVParser>(csv_config);
        params = {{kLeftTable, table}, {kMDLHS, lhs}, {kMDRHS, rhs}};
    }
};

class TestMDVerifier : public ::testing::TestWithParam<MDVerifierParams> {};

class TestMDVerifierHighlights : public ::testing::TestWithParam<MDVerifierHighlightsParams> {};

static std::unique_ptr<algos::md::MDVerifier> CreateMDVerifier(algos::StdParamsMap const& map) {
    auto mp = algos::StdParamsMap(map);
    return algos::CreateAndLoadAlgorithm<algos::md::MDVerifier>(mp);
}

static bool ExecuteAlgo(algos::md::MDVerifier& md_verifier) {
    md_verifier.Execute();
    return md_verifier.GetResult();
}

TEST_P(TestMDVerifier, DefaultCase) {
    auto const& params = GetParam().params;
    auto verifier = CreateMDVerifier(params);

    auto md_result = ExecuteAlgo(*verifier);

    ASSERT_EQ(GetParam().expected, md_result);
    ASSERT_EQ(verifier->GetRHSSuggestion(), GetParam().suggestions);
}

TEST_P(TestMDVerifierHighlights, DefaultCase) {
    using Highlight = algos::md::MDHighlights::Highlight;
    auto const& params = GetParam().params;
    auto verifier = CreateMDVerifier(params);

    auto md_result = ExecuteAlgo(*verifier);

    ASSERT_EQ(GetParam().expected, md_result);
    std::vector<Highlight> highlights = verifier->GetHighlights();
    ASSERT_EQ(highlights.size(), GetParam().highlights.size());
    for (std::size_t i = 0; i < highlights.size(); ++i) {
        ASSERT_EQ(highlights[i].ToString(), GetParam().highlights[i]);
    }
}

auto const kEps = std::numeric_limits<DecisionBoundary>::epsilon();

INSTANTIATE_TEST_SUITE_P(
        TestMDVerifierSuite, TestMDVerifier,
        ::testing::Values(
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75),
                        true, 0.75),
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                 0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.125),
                         algos::md::MDVerifierColumnSimilarityClassifier(
                                 3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                1. / 5.),
                        true, 1. / 5.),
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                 0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.125),
                         algos::md::MDVerifierColumnSimilarityClassifier(
                                 2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.5),
                        false, 1. / 5.),
                MDVerifierParams(kAnimalsBeverages,
                                 {algos::md::MDVerifierColumnSimilarityClassifier(
                                         2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                         0.75 + kEps)},
                                 algos::md::MDVerifierColumnSimilarityClassifier(
                                         3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                         0.75),
                                 true, 0.75),
                MDVerifierParams(kAnimalsBeverages,
                                 {algos::md::MDVerifierColumnSimilarityClassifier(
                                         2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                         0.75)},
                                 algos::md::MDVerifierColumnSimilarityClassifier(
                                         3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                         0.75 + kEps),
                                 false, 0.75)));

INSTANTIATE_TEST_SUITE_P(
        TestMDVerifierHighlightsSuite, TestMDVerifierHighlights,
        ::testing::Values(
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75),
                        true, {}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                 0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.125),
                         algos::md::MDVerifierColumnSimilarityClassifier(
                                 3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                1. / 5.),
                        true, {}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                 0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.125),
                         algos::md::MDVerifierColumnSimilarityClassifier(
                                 2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                0, 0, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.5),
                        false,
                        {"levenshtein(0, 0) violates MD in 2 row of left table and 3 row of right "
                         "table with similarity 0.2 and decision boundary 0.5",
                         "levenshtein(0, 0) violates MD in 3 row of left table and 2 row of right "
                         "table with similarity 0.2 and decision boundary 0.5"}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                0.75 + kEps)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75),
                        true, {}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::MDVerifierColumnSimilarityClassifier(
                                2, 2, std::make_shared<algos::md::LevenshteinSimilarity>(), 0.75)},
                        algos::md::MDVerifierColumnSimilarityClassifier(
                                3, 3, std::make_shared<algos::md::LevenshteinSimilarity>(),
                                0.75 + kEps),
                        false,
                        {"levenshtein(3, 3) violates MD in 0 row of left table and 1 row of right "
                         "table with similarity 0.75 and decision boundary 0.75",
                         "levenshtein(3, 3) violates MD in 1 row of left table and 0 row of right "
                         "table with similarity 0.75 and decision boundary 0.75"})));

}  // namespace tests