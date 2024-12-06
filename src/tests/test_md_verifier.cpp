#include <limits>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/md/md_verifier/md_verifier.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
namespace onam = config::names;
using MDVerifier = algos::md::MDVerifier;
using DecisionBoundary = model::md::DecisionBoundary;

struct MDVerifierParams {
    using SimilarityMeasure = algos::md::SimilarityMeasure;

    algos::StdParamsMap params;
    bool const expected;
    std::set<std::pair<int, int>> expected_pairs;
    std::vector<DecisionBoundary> suggestions;

    MDVerifierParams(CSVConfig const& csv_config, config::IndicesType lhs_indices,
                     config::IndicesType rhs_indices,
                     std::vector<model::md::DecisionBoundary> lhs_desicion_bondaries,
                     std::vector<model::md::DecisionBoundary> rhs_desicion_bondaries,
                     std::vector<std::shared_ptr<SimilarityMeasure>> lhs_similarity_measures,
                     std::vector<std::shared_ptr<SimilarityMeasure>> rhs_similarity_measures,
                     bool const expected, std::set<std::pair<int, int>> expected_pairs,
                     std::vector<DecisionBoundary> suggestions)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kMDLhsDecisionBoundaries, std::move(lhs_desicion_bondaries)},
                  {onam::kMDRhsDecisionBoundaries, std::move(rhs_desicion_bondaries)},
                  {onam::kMDLhsSimilarityMeasures, std::move(lhs_similarity_measures)},
                  {onam::kMDRhsSimilarityMeasures, std::move(rhs_similarity_measures)},
                  {onam::kEqualNulls, true},
                  {onam::kDistFromNullIsInfinity, false}}),
          expected(expected),
          expected_pairs(std::move(expected_pairs)),
          suggestions(std::move(suggestions)) {}
};

class TestMDVerifier : public ::testing::TestWithParam<MDVerifierParams> {};

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

    ASSERT_EQ(verifier->GetPairsViolatingMD(), GetParam().expected_pairs);
    ASSERT_EQ(verifier->GetRhsSuggestions(), GetParam().suggestions);
}

auto constexpr eps = std::numeric_limits<DecisionBoundary>::epsilon();

INSTANTIATE_TEST_SUITE_P(
        TestMDVerifierSuite, TestMDVerifier,
        ::testing::Values(MDVerifierParams(kDrunkAnimals, {2}, {3}, {0.75}, {0.75},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           true, {}, {0.75}),
                          MDVerifierParams(kDrunkAnimals, {2}, {3}, {0.75 + eps}, {0.75},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           true, {}, {0.75}),
                          MDVerifierParams(kDrunkAnimals, {2}, {3}, {0.75}, {0.75 + eps},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           false, {{0, 1}}, {0.75}),
                          MDVerifierParams(kDrunkAnimals, {2, 3}, {2, 3}, {0.75, 0.75},
                                           {0.75, 0.75},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>(),
                                            std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           {std::make_shared<algos::md::LevenshteinSimilarity>(),
                                            std::make_shared<algos::md::LevenshteinSimilarity>()},
                                           true, {}, {0.75, 0.75})));

}  // namespace tests