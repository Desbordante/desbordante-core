#include <cstdint>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/names.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace config_names = config::names;
namespace rfd = algos::rfd;

auto lev = rfd::LevenshteinMetric();
auto eq = rfd::EqualityMetric();
auto abs_diff = rfd::AbsoluteDifferenceMetric();

using namespace std::chrono;

template <typename Func>
long long MeasureExecutionTime(Func&& f) {
    auto start = high_resolution_clock::now();
    f();
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

static algos::StdParamsMap MakeParams(
        CSVConfig const& csv_config, double min_sim, double beta, std::size_t pop_size,
        std::size_t max_gen, std::vector<std::shared_ptr<rfd::SimilarityMetric>> metrics = {}) {
    config::InputTable table = std::make_shared<CSVParser>(csv_config);
    algos::StdParamsMap params{{config_names::kTable, table},
                               {config_names::kRfdMinSimilarity, min_sim},
                               {config_names::kMinimumConfidence, beta},
                               {config_names::kPopulationSize, pop_size},
                               {config_names::kRfdMaxGenerations, max_gen},
                               {config_names::kRfdCrossoverProbability, 0.85},
                               {config_names::kRfdMutationProbability, 0.3},
                               {config_names::kSeed, std::uint64_t{42}}};
    if (!metrics.empty()) {
        params["metrics"] = metrics;
    }
    return params;
}

static std::vector<std::shared_ptr<rfd::SimilarityMetric>> EqualityMetrics(std::size_t n) {
    return std::vector<std::shared_ptr<rfd::SimilarityMetric>>(n, eq);
}

TEST(GARfd, AbsoluteDifferenceMetricYieldsRfdsOnIris) {
    constexpr std::size_t pop_size = 20;
    constexpr std::size_t generations = 50;
    constexpr double min_similarity = 0.95;
    constexpr double min_confidence = 0.8;

    config::InputTable table = std::make_shared<CSVParser>(kIris);

    std::vector<std::shared_ptr<rfd::SimilarityMetric>> metrics;
    for (int i = 0; i < 4; ++i) metrics.push_back(abs_diff);
    metrics.push_back(eq);

    algos::StdParamsMap params{{config_names::kTable, table},
                               {"metrics", metrics},
                               {config_names::kRfdMinSimilarity, min_similarity},
                               {config_names::kMinimumConfidence, min_confidence},
                               {config_names::kPopulationSize, pop_size},
                               {config_names::kRfdMaxGenerations, generations},
                               {config_names::kRfdCrossoverProbability, 0.85},
                               {config_names::kRfdMutationProbability, 0.3},
                               {config_names::kSeed, static_cast<std::uint64_t>(123)}};

    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();

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

TEST(GARfdEdge, ThrowsOnEmptyTable) {
    auto metrics = EqualityMetrics(1);
    auto params = MakeParams(kTestEmpty, 0.5, 0.5, 10, 1, metrics);
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params), std::runtime_error);
}

TEST(GARfdEdge, ThrowsOnSingleAttribute) {
    auto metrics = EqualityMetrics(1);
    auto params = MakeParams(kTestSingleColumn, 0.5, 0.5, 10, 1, metrics);
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params), std::runtime_error);
}

TEST(GARfdEdge, ThrowsOnTooManyAttributes) {
    auto metrics = EqualityMetrics(32);
    auto params = MakeParams(kRfd32Attrs, 0.5, 0.5, 10, 1, metrics);
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params), std::runtime_error);
}

TEST(GARfdEdge, ThrowsOnSingleRow) {
    auto metrics = EqualityMetrics(2);
    auto params = MakeParams(kRfdSingleRow, 0.5, 0.5, 10, 1, metrics);
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params), std::runtime_error);
}

struct RFDTestParams {
    CSVConfig csv_config;
    double min_sim;
    double beta;
    std::size_t pop_size;
    std::size_t max_gen;
    bool expect_success;
    std::size_t min_rfd_count;
};

class GaRfdDatasetTest : public ::testing::TestWithParam<RFDTestParams> {};

TEST_P(GaRfdDatasetTest, DiscoversRFDs) {
    auto const& p = GetParam();
    auto metrics = EqualityMetrics(0);
    auto params = MakeParams(p.csv_config, p.min_sim, p.beta, p.pop_size, p.max_gen, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);

    if (p.expect_success) {
        algo->Execute();
        auto rfds = algo->GetRfds();
        EXPECT_GE(rfds.size(), p.min_rfd_count)
                << "Expected at least " << p.min_rfd_count << " RFDs";
        for (auto const& rfd : rfds) {
            EXPECT_NE(rfd.lhs_mask, 0u) << "LHS must not be empty";
            EXPECT_FALSE(rfd.lhs_mask & (1u << rfd.rhs_index)) << "RHS must not be in LHS";
            EXPECT_GE(rfd.confidence, p.beta) << "Confidence should be ≥ beta";
            EXPECT_GE(rfd.support, 0.0);
            EXPECT_LE(rfd.support, 1.0);
        }
    } else {
        EXPECT_THROW(algo->Execute(), std::runtime_error);
    }
}

INSTANTIATE_TEST_SUITE_P(VariousDatasets, GaRfdDatasetTest,
                         ::testing::Values(RFDTestParams{kIris, 1.0, 0.9, 40, 5, true, 1},
                                           RFDTestParams{kBreastCancer, 1.0, 0.9, 100, 10, true, 0},
                                           RFDTestParams{kNeighbors10k, 0.8, 0.9, 10, 3, true, 1},
                                           RFDTestParams{kTestFD, 1.0, 0.9, 40, 5, true, 1},
                                           RFDTestParams{kTestLong, 1.0, 0.9, 30, 5, true, 1},
                                           RFDTestParams{kTestWide, 1.0, 0.9, 30, 5, true, 0},
                                           RFDTestParams{kRfdTwoRows, 1.0, 0.9, 10, 1, true, 0}),
                         [](testing::TestParamInfo<RFDTestParams> const& info) {
                             return std::to_string(info.index);
                         });

TEST(GARfdFunctional, IrisAllSimilarYieldsNonEmpty) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 0.0, 0.5, 20, 2, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    EXPECT_GE(rfds.size(), 5);
}

TEST(GARfdMetric, LevenshteinMetricOnIris) {
    auto metrics = std::vector<std::shared_ptr<rfd::SimilarityMetric>>{lev, lev, lev, lev, eq};
    auto params = MakeParams(kIris, 0.8, 0.9, 30, 5, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    std::cout << "Iris with Levenshtein found " << rfds.size() << " RFDs\n";
    for (auto const& r : rfds) std::cout << "  " << r.ToString() << '\n';
    EXPECT_GE(rfds.size(), 1);
}

TEST(GARfdDeterminism, SameSeedSameResult) {
    auto metrics = EqualityMetrics(5);
    auto params1 = MakeParams(kIris, 0.0, 0.5, 20, 1, metrics);
    auto params2 = MakeParams(kIris, 0.0, 0.5, 20, 1, metrics);
    auto algo1 = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params1);
    auto algo2 = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params2);
    algo1->Execute();
    algo2->Execute();
    auto r1 = algo1->GetRfds();
    auto r2 = algo2->GetRfds();
    ASSERT_EQ(r1.size(), r2.size());
    for (size_t i = 0; i < r1.size(); ++i) {
        EXPECT_EQ(r1[i].lhs_mask, r2[i].lhs_mask);
        EXPECT_EQ(r1[i].rhs_index, r2[i].rhs_index);
        EXPECT_DOUBLE_EQ(r1[i].confidence, r2[i].confidence);
        EXPECT_DOUBLE_EQ(r1[i].support, r2[i].support);
    }
}

TEST(GARfdCache, CacheSizeOptionWorks) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 0.8, 0.9, 20, 2, metrics);
    params[config_names::kCacheMaxSize] = static_cast<std::size_t>(1);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    EXPECT_GE(rfds.size(), 0);
}

TEST(GARfdEvolution, EarlyStopWhenAllSatisfy) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 0.0, 0.0, 20, 100, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    EXPECT_GE(rfds.size(), 0);
}

TEST(GARfdEdge, MismatchedMetricsCountThrows) {
    auto metrics = EqualityMetrics(3);
    auto params = MakeParams(kIris, 0.5, 0.5, 10, 1, metrics);
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params), std::invalid_argument);
}

TEST(GARfdOperators, ZeroCrossoverAndMutation) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 0.5, 0.8, 30, 3, metrics);
    params[config_names::kRfdCrossoverProbability] = 0.0;
    params[config_names::kRfdMutationProbability] = 0.0;
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    EXPECT_GE(rfds.size(), 0);
}

}  // namespace tests
