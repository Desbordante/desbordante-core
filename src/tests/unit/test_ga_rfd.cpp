#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/config/names.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace config_names = config::names;
namespace rfd = algos::rfd;

TEST(GARfd, AllPairsSimilarConfAndSupportAreOne) {
    constexpr std::size_t pop_size = 5;
    CSVConfig const& csv_config = kIris;
    config::InputTable table = std::make_unique<CSVParser>(csv_config);
    std::vector<std::shared_ptr<rfd::SimilarityMetric>> metrics(5, rfd::EqualityMetric());

    algos::StdParamsMap params{
            {config_names::kTable, table},
            { "metrics", metrics},
            {config_names::kRfdMinSimilarity, 0.0},
            {config_names::kMinimumConfidence, 0.0},
            {config_names::kPopulationSize, pop_size},
            {config_names::kRfdMaxGenerations, static_cast<std::size_t>(0)},
            {config_names::kRfdCrossoverProbability, 0.85},
            {config_names::kRfdMutationProbability, 0.3},
            {config_names::kSeed, static_cast<std::uint64_t>(123)}};

    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    auto time_ms = algo->Execute();
    (void)time_ms;

    auto const& rfds = algo->GetRfds();
    std::cout << "Discovered RFDs:\n";
    for (auto const& rfd : rfds) {
        std::cout << rfd.ToString() << '\n';
    }
    
    for (auto const& rfd : rfds) {
        EXPECT_NE(rfd.lhs_mask, 0u);
        EXPECT_FALSE(rfd.lhs_mask & (1u << rfd.rhs_index));
        EXPECT_DOUBLE_EQ(rfd.support, 1.0);
        EXPECT_DOUBLE_EQ(rfd.confidence, 1.0);
        EXPECT_GE(rfd.confidence, 0.5);
    }
}

TEST(GARfd, AbsoluteDifferenceMetricYieldsRfdsOnIris) {
    constexpr std::size_t pop_size = 20;
    constexpr std::size_t generations = 50;
    constexpr double min_similarity = 0.95;
    constexpr double min_confidence = 0.8;

    CSVConfig const& csv_config = kIris;
    config::InputTable table = std::make_unique<CSVParser>(csv_config);

    std::vector<std::shared_ptr<rfd::SimilarityMetric>> metrics;
    for (int i = 0; i < 4; ++i)
        metrics.push_back(rfd::AbsoluteDifferenceMetric());
    metrics.push_back(rfd::EqualityMetric());

    algos::StdParamsMap params{
            {config_names::kTable, table},
            { "metrics", metrics},
            {config_names::kRfdMinSimilarity, min_similarity},
            {config_names::kMinimumConfidence, min_confidence},
            {config_names::kPopulationSize, pop_size},
            {config_names::kRfdMaxGenerations, generations},
            {config_names::kRfdCrossoverProbability, 0.85},
            {config_names::kRfdMutationProbability, 0.3},
            {config_names::kSeed, static_cast<std::uint64_t>(123)}};

    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    auto time_ms = algo->Execute();
    (void)time_ms;

    auto const& rfds = algo->GetRfds();
    std::cout << "Discovered RFDs (absolute diff metric):\n";
    for (auto const& rfd : rfds) {
        std::cout << rfd.ToString() << '\n';
    }

    ASSERT_FALSE(rfds.empty()) << "Expected at least one RFD to be found";
    for (auto const& rfd : rfds) {
        EXPECT_NE(rfd.lhs_mask, 0u);
        EXPECT_FALSE(rfd.lhs_mask & (1u << rfd.rhs_index));
        EXPECT_GE(rfd.confidence, min_confidence);
        EXPECT_LE(rfd.confidence, 1.0);
        EXPECT_GE(rfd.support, 0.0);
        EXPECT_LE(rfd.support, 1.0);
    }
}

}
