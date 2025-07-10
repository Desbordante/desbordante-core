#include <limits>
#include <memory>
#include <set>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match_impl.h"
#include "algorithms/md/hymd/preprocessing/column_matches/levenshtein.h"
#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
using namespace config::names;
using MDVerifier = algos::md::MDVerifier;
using DecisionBoundary = model::md::DecisionBoundary;
using ColumnIdentifier = algos::hymd::preprocessing::column_matches::ColumnIdentifier;

struct MDVerifierParams {
    algos::StdParamsMap params;
    bool const expected;
    DecisionBoundary suggestions;

    MDVerifierParams(CSVConfig const& csv_config,
                     std::vector<algos::md::ColumnSimilarityClassifier> lhs,
                     algos::md::ColumnSimilarityClassifier rhs, bool const expected,
                     DecisionBoundary suggestions)
        : expected(expected), suggestions(suggestions) {
        config::InputTable table = std::make_unique<CSVParser>(csv_config);
        params = {{kLeftTable, table}, {kMDLHS, lhs}, {kMDRHS, rhs}};
    }
};

struct MDVerifierHighlightsParams {
    algos::StdParamsMap params;
    bool const expected;
    std::set<std::string> highlights;

    MDVerifierHighlightsParams(CSVConfig const& csv_config,
                               std::vector<algos::md::ColumnSimilarityClassifier> lhs,
                               algos::md::ColumnSimilarityClassifier rhs, bool const expected,
                               std::set<std::string> highlights)
        : expected(expected), highlights(std::move(highlights)) {
        config::InputTable table = std::make_unique<CSVParser>(csv_config);
        params = {{kLeftTable, table}, {kMDLHS, lhs}, {kMDRHS, rhs}};
    }
};

struct MDVerifierSuggestionsParams {
    algos::StdParamsMap params;
    bool const expected;
    std::string md_string;

    MDVerifierSuggestionsParams(CSVConfig const& csv_config,
                                std::vector<algos::md::ColumnSimilarityClassifier> lhs,
                                algos::md::ColumnSimilarityClassifier rhs, bool const expected,
                                std::string md_string)
        : expected(expected), md_string(std::move(md_string)) {
        config::InputTable table = std::make_unique<CSVParser>(csv_config);
        params = {{kLeftTable, table}, {kMDLHS, lhs}, {kMDRHS, rhs}};
    }
};

class TestMDVerifier : public ::testing::TestWithParam<MDVerifierParams> {};

class TestMDVerifierHighlights : public ::testing::TestWithParam<MDVerifierHighlightsParams> {};

class TestMDVerifierSuggestions : public ::testing::TestWithParam<MDVerifierSuggestionsParams> {};

static std::unique_ptr<algos::md::MDVerifier> CreateMDVerifier(algos::StdParamsMap const& map) {
    algos::StdParamsMap mp = algos::StdParamsMap(map);
    return algos::CreateAndLoadAlgorithm<algos::md::MDVerifier>(mp);
}

static bool ExecuteAlgo(algos::md::MDVerifier& md_verifier) {
    md_verifier.Execute();
    return md_verifier.GetResult();
}

TEST_P(TestMDVerifier, DefaultCase) {
    algos::StdParamsMap const& params = GetParam().params;
    std::unique_ptr<algos::md::MDVerifier> verifier = CreateMDVerifier(params);

    bool md_result = ExecuteAlgo(*verifier);

    ASSERT_EQ(GetParam().expected, md_result);
    ASSERT_DOUBLE_EQ(verifier->GetTrueRhsDecisionBoundary(), GetParam().suggestions);
}

TEST_P(TestMDVerifierHighlights, DefaultCase) {
    using Highlight = algos::md::MDHighlights::Highlight;
    algos::StdParamsMap const& params = GetParam().params;
    std::unique_ptr<algos::md::MDVerifier> verifier = CreateMDVerifier(params);

    bool md_result = ExecuteAlgo(*verifier);

    ASSERT_EQ(GetParam().expected, md_result);
    std::vector<Highlight> highlights = verifier->GetHighlights();
    ASSERT_EQ(highlights.size(), GetParam().highlights.size());
    for (Highlight const& highlight : highlights) {
        std::cout << highlight.ToString() << std::endl;
        ASSERT_TRUE(GetParam().highlights.contains(highlight.ToString()));
    }
}

TEST_P(TestMDVerifierSuggestions, DefaultCase) {
    algos::StdParamsMap const& params = GetParam().params;
    std::unique_ptr<algos::md::MDVerifier> verifier = CreateMDVerifier(params);

    bool md_result = ExecuteAlgo(*verifier);

    ASSERT_EQ(GetParam().expected, md_result);
    std::vector<model::MD> suggested_mds = verifier->GetMDSuggestion();
    ASSERT_EQ(suggested_mds.size(), 1);
    std::string rhs_suggested_md_string = suggested_mds.front().ToStringActiveLhsOnly();
    ASSERT_EQ(rhs_suggested_md_string, GetParam().md_string);
    if (md_result) {
        ASSERT_EQ(rhs_suggested_md_string, verifier->GetInputMD().ToStringActiveLhsOnly());
    }
}

double const kEps = std::numeric_limits<DecisionBoundary>::epsilon();

INSTANTIATE_TEST_SUITE_P(
        TestMDVerifierSuite, TestMDVerifier,
        ::testing::Values(
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75),
                        true, 0.75),
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(0), static_cast<model::Index>(0),
                                         0.0),
                                 0.125),
                         algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(3), static_cast<model::Index>(3),
                                         0.0),
                                 0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                1 / 5.),
                        true, 1 / 5.),
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(0), static_cast<model::Index>(0),
                                         0.0),
                                 0.125),
                         algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(2), static_cast<model::Index>(2),
                                         0.0),
                                 0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(0), static_cast<model::Index>(0),
                                        0.0),
                                0.5),
                        false, 1 / 5.),
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75 + kEps)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75),
                        true, 0.75),
                MDVerifierParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75 + kEps),
                        false, 0.75),
                MDVerifierParams(
                        kMDTrivial,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                1.0)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                1.0),
                        true, 1.0),
                MDVerifierParams(
                        kMDTrivial,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                1.0)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(3),
                                        0.0),
                                1.0),
                        false, 0.0)));

INSTANTIATE_TEST_SUITE_P(
        TestMDVerifierHighlightsSuite, TestMDVerifierHighlights,
        ::testing::Values(
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75),
                        true, {}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(0), static_cast<model::Index>(0),
                                         0.0),
                                 0.125),
                         algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(3), static_cast<model::Index>(3),
                                         0.0),
                                 0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                1 / 5.),
                        true, {}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(0), static_cast<model::Index>(0),
                                         0.0),
                                 0.125),
                         algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(2), static_cast<model::Index>(2),
                                         0.0),
                                 0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(0), static_cast<model::Index>(0),
                                        0.0),
                                0.5),
                        false,
                        {"levenshtein(name, name) violates MD in 2 row of left table and 3 row of "
                         "right "
                         "table with similarity 0.2 and decision boundary 0.5",
                         "levenshtein(name, name) violates MD in 3 row of left table and 2 row of "
                         "right "
                         "table with similarity 0.2 and decision boundary 0.5"}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75 + kEps)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75),
                        true, {}),
                MDVerifierHighlightsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75 + kEps),
                        false,
                        {"levenshtein(diet, diet) violates MD in 0 row of left table and 1 row of "
                         "right "
                         "table with similarity 0.75 and decision boundary 0.75",
                         "levenshtein(diet, diet) violates MD in 1 row of left table and 0 row of "
                         "right "
                         "table with similarity 0.75 and decision boundary 0.75"}),
                MDVerifierHighlightsParams(
                        kMDTrivial,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                1.0)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                1.0),
                        true, {}),
                MDVerifierHighlightsParams(
                        kMDTrivial,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                1.0)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(3),
                                        0.0),
                                1.0),
                        false,
                        {"levenshtein(animal, diet) violates MD in 0 row of left table and 0 row "
                         "of right table with similarity 0 and decision boundary 1"})));

INSTANTIATE_TEST_SUITE_P(
        TestMDVerifierSuggestionsSuite, TestMDVerifierSuggestions,
        ::testing::Values(
                MDVerifierSuggestionsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75),
                        true,
                        "[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, "
                        "diet)>=0.75"),
                MDVerifierSuggestionsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(0), static_cast<model::Index>(0),
                                         0.0),
                                 0.125),
                         algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(3), static_cast<model::Index>(3),
                                         0.0),
                                 0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                1 / 5.),
                        true,
                        "[ levenshtein(name, name)>=0.125 | levenshtein(diet, diet)>=0.75 ] -> "
                        "levenshtein(diet, diet)>=0.2"),
                MDVerifierSuggestionsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(0), static_cast<model::Index>(0),
                                         0.0),
                                 0.125),
                         algos::md::ColumnSimilarityClassifier(
                                 std::make_shared<
                                         algos::hymd::preprocessing::column_matches::Levenshtein>(
                                         static_cast<model::Index>(2), static_cast<model::Index>(2),
                                         0.0),
                                 0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(0), static_cast<model::Index>(0),
                                        0.0),
                                0.5),
                        false,
                        "[ levenshtein(name, name)>=0.125 | levenshtein(animal, animal)>=0.75 "
                        "] -> "
                        "levenshtein(name, name)>=0.2"),
                MDVerifierSuggestionsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75 + kEps)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75),
                        true,
                        "[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, "
                        "diet)>=0.75"),
                MDVerifierSuggestionsParams(
                        kAnimalsBeverages,
                        {algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(2), static_cast<model::Index>(2),
                                        0.0),
                                0.75)},
                        algos::md::ColumnSimilarityClassifier(
                                std::make_shared<
                                        algos::hymd::preprocessing::column_matches::Levenshtein>(
                                        static_cast<model::Index>(3), static_cast<model::Index>(3),
                                        0.0),
                                0.75 + kEps),
                        false,
                        "[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, "
                        "diet)>=0.75")));

}  // namespace tests
