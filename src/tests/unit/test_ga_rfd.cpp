#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/distance_metric.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/config/names.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "core/util/custom_metric/custom_metric.h"
#include "core/util/logger.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
using namespace algos::rfd;
using namespace config::names;

auto const kLev = LevenshteinMetric();
auto const kEq = EqualityMetric();
auto const kAbsDiff = AbsoluteDifferenceMetric();

static algos::StdParamsMap MakeParams(CSVConfig const& csv_config, std::vector<double> min_sim,
                                      double min_confidence, std::size_t pop_size,
                                      std::size_t max_gen, config::CustomMetricsType metrics = {}) {
    config::InputTable table = std::make_shared<CSVParser>(csv_config);
    algos::StdParamsMap params{{kTable, table},
                               {kRfdMinSimilarity, min_sim},
                               {kRfdMinimumConfidence, min_confidence},
                               {kPopulationSize, pop_size},
                               {kRfdMaxGenerations, max_gen},
                               {kRfdCrossoverProbability, 0.85},
                               {kRfdMutationProbability, 0.3},
                               {kSeed, std::uint32_t{42}}};
    if (!metrics.empty()) {
        params[kCustomMetrics] = metrics;
    }
    return params;
}

static algos::StdParamsMap MakeParams(CSVConfig const& csv_config, double min_sim,
                                      double min_confidence, std::size_t pop_size,
                                      std::size_t max_gen, config::CustomMetricsType metrics = {}) {
    return MakeParams(csv_config, std::vector<double>{min_sim}, min_confidence, pop_size, max_gen,
                      metrics);
}

static config::CustomMetricsType EqualityMetrics(std::size_t n) {
    return config::CustomMetricsType(n, kEq);
}

static void ExpectValidRfds(std::vector<RFD> const& rfds, double min_confidence) {
    for (auto const& rfd : rfds) {
        EXPECT_FALSE(rfd.lhs.empty()) << "LHS must not be empty";
        EXPECT_EQ(rfd.lhs.size(), rfd.lhs_indices.size()) << "LHS names and indices must match";
        EXPECT_FALSE(rfd.rhs.empty()) << "RHS must name a column";
        EXPECT_TRUE(std::find(rfd.lhs_indices.begin(), rfd.lhs_indices.end(), rfd.rhs_index) ==
                    rfd.lhs_indices.end())
                << "RHS must not be in LHS";
        EXPECT_GE(rfd.confidence, min_confidence);
        EXPECT_LE(rfd.confidence, 1.0);
        EXPECT_GE(rfd.support, 0.0);
        EXPECT_LE(rfd.support, 1.0);
    }
}

static std::vector<std::string> SortedRfdStrings(std::vector<RFD> const& rfds) {
    std::vector<std::string> strings;
    for (auto const& rfd : rfds) strings.push_back(rfd.ToString());
    std::sort(strings.begin(), strings.end());
    return strings;
}

// -----------------------------------------------------------
TEST(GARfd, AbsoluteDifferenceMetricYieldsRfdsOnIris) {
    constexpr std::size_t pop_size = 20;
    constexpr std::size_t generations = 50;
    constexpr double min_similarity = 0.95;
    constexpr double min_confidence = 0.8;

    config::InputTable table = std::make_shared<CSVParser>(kIris);

    config::CustomMetricsType metrics;
    for (int i = 0; i < 4; ++i) metrics.push_back(kAbsDiff);
    metrics.push_back(kEq);

    std::vector<double> sim_vec{min_similarity};

    algos::StdParamsMap params{{kTable, table},
                               {kCustomMetrics, metrics},
                               {kRfdMinSimilarity, sim_vec},
                               {kRfdMinimumConfidence, min_confidence},
                               {kPopulationSize, pop_size},
                               {kRfdMaxGenerations, generations},
                               {kRfdCrossoverProbability, 0.85},
                               {kRfdMutationProbability, 0.3},
                               {kSeed, std::uint32_t{123}}};

    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();

    auto const& rfds = algo->GetRfds();
    LOG_DEBUG("Discovered {} RFDs (absolute diff metric)", rfds.size());
    for (auto const& rfd : rfds) {
        LOG_DEBUG("  {}", rfd.ToString());
    }

    ASSERT_FALSE(rfds.empty()) << "Expected at least one RFD to be found";
    ExpectValidRfds(rfds, min_confidence);
}

struct RFDTestParams {
    CSVConfig csv_config;
    double min_sim;
    double min_confidence;
    std::size_t pop_size;
    std::size_t max_gen;
    bool expect_success;
    std::size_t min_rfd_count;
};

class GaRfdDatasetTest : public ::testing::TestWithParam<RFDTestParams> {};

TEST_P(GaRfdDatasetTest, DiscoversRFDs) {
    auto const& p = GetParam();
    auto tmp_parser = std::make_shared<CSVParser>(p.csv_config);
    std::size_t ncols = tmp_parser->GetNumberOfColumns();
    std::vector<double> sim_vec(ncols, p.min_sim);

    // TestFD.csv col 3 mixes numbers, '-' and strings, so the column is
    // Mixed-typed and the default metric (equality) is used explicitly
    // to avoid metrizable-type issues
    config::CustomMetricsType metrics;
    if (p.csv_config.path.filename() == "TestFD.csv") {
        metrics = config::CustomMetricsType(ncols, kEq);
    }
    auto params =
            MakeParams(p.csv_config, sim_vec, p.min_confidence, p.pop_size, p.max_gen, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);

    if (p.expect_success) {
        algo->Execute();
        auto rfds = algo->GetRfds();
        EXPECT_GE(rfds.size(), p.min_rfd_count)
                << "Expected at least " << p.min_rfd_count << " RFDs";
        ExpectValidRfds(rfds, p.min_confidence);
    } else {
        EXPECT_THROW(algo->Execute(), std::exception);
    }
}

INSTANTIATE_TEST_SUITE_P(VariousDatasets, GaRfdDatasetTest,
                         ::testing::Values(RFDTestParams{kIris, 1.0, 0.9, 40, 5, true, 1},
                                           RFDTestParams{kBreastCancer, 1.0, 0.9, 100, 10, true, 0},
                                           RFDTestParams{kNeighbors10k, 0.5, 0.9, 10, 3, true, 1},
                                           RFDTestParams{kTestFD, 1.0, 0.9, 40, 5, true, 1},
                                           RFDTestParams{kTestLong, 1.0, 0.9, 30, 5, true, 1},
                                           RFDTestParams{kTestWide, 1.0, 0.9, 30, 5, true, 0},
                                           RFDTestParams{kRfdTwoRows, 1.0, 0.9, 10, 1, true, 0}));

TEST(GARfdFunctional, IrisAllSimilarYieldsNonEmpty) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 1.0, 0.5, 20, 2, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    EXPECT_GE(rfds.size(), 5);
}

TEST(GARfdNames, MinedRfdsCarryColumnNamesAndIndices) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 1.0, 0.5, 20, 2, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ASSERT_FALSE(rfds.empty());
    for (auto const& rfd : rfds) {
        EXPECT_EQ(rfd.lhs.size(), rfd.lhs_indices.size());
        for (std::size_t i = 0; i < rfd.lhs.size(); ++i) {
            EXPECT_LT(rfd.lhs_indices[i], 5u);
            EXPECT_FALSE(rfd.lhs[i].empty());
        }
        EXPECT_LT(rfd.rhs_index, 5u);
        EXPECT_FALSE(rfd.rhs.empty());
    }
}

TEST(GARfdMetric, LevenshteinMetricOnIris) {
    auto metrics = config::CustomMetricsType{kLev, kLev, kLev, kLev, kEq};
    auto params = MakeParams(kIris, 0.8, 0.9, 30, 5, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    LOG_DEBUG("Iris with Levenshtein found {} RFDs", rfds.size());
    for (auto const& r : rfds) LOG_DEBUG("  {}", r.ToString());
    EXPECT_GE(rfds.size(), 1);
}

TEST(GARfdDeterminism, SameSeedSameResult) {
    auto metrics = EqualityMetrics(5);
    auto params1 = MakeParams(kIris, 1.0, 0.5, 20, 1, metrics);
    auto params2 = MakeParams(kIris, 1.0, 0.5, 20, 1, metrics);
    auto algo1 = algos::CreateAndLoadAlgorithm<GaRfd>(params1);
    auto algo2 = algos::CreateAndLoadAlgorithm<GaRfd>(params2);
    algo1->Execute();
    algo2->Execute();
    auto r1 = algo1->GetRfds();
    auto r2 = algo2->GetRfds();
    ASSERT_EQ(r1.size(), r2.size());
    std::set<RFD> set1(r1.begin(), r1.end());
    std::set<RFD> set2(r2.begin(), r2.end());
    EXPECT_EQ(set1, set2);
}

TEST(GARfdSeed, DifferentSeedsGiveValidRfds) {
    for (std::uint32_t seed : {1u, 42u, 123456u}) {
        auto metrics = EqualityMetrics(5);
        auto params = MakeParams(kIris, 1.0, 0.5, 20, 2, metrics);
        params[kSeed] = seed;
        auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
        algo->Execute();
        ExpectValidRfds(algo->GetRfds(), 0.5);
    }
}

TEST(GARfdSeed, UnsetSeedRunsWithRandomDefault) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 1.0, 0.5, 20, 2, metrics);
    params.erase(std::string(kSeed));
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    ExpectValidRfds(algo->GetRfds(), 0.5);
}

TEST(GARfdCache, CacheSizeOptionWorks) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 1.0, 0.9, 20, 2, metrics);
    auto reference = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    reference->Execute();

    params[kCacheMaxSize] = static_cast<std::size_t>(1);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    // The cache is a pure memoization layer: shrinking it must not change results
    EXPECT_EQ(SortedRfdStrings(rfds), SortedRfdStrings(reference->GetRfds()));
    ExpectValidRfds(rfds, 0.9);
}

TEST(GARfdEvolution, EarlyStopWhenAllSatisfy) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 1.0, 0.0, 20, 100, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ExpectValidRfds(rfds, 0.0);
}

TEST(GARfdEdge, MismatchedMetricsCountThrows) {
    auto metrics = EqualityMetrics(3);
    auto params = MakeParams(kIris, 1.0, 0.5, 10, 1, metrics);
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<GaRfd>(params), std::exception);
}

TEST(GARfdMetric, AbsoluteThresholdMetricWithTolerances) {
    config::CustomMetricsType metrics{
            AbsoluteThresholdMetricFactory(0.5), AbsoluteThresholdMetricFactory(0.5),
            AbsoluteThresholdMetricFactory(0.5), AbsoluteThresholdMetricFactory(0.5), kEq};
    auto params = MakeParams(kIris, std::vector<double>(5, 1.0), 0.5, 20, 2, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    EXPECT_FALSE(rfds.empty());
    ExpectValidRfds(rfds, 0.5);
}

TEST(GARfdOperators, ZeroCrossoverAndMutation) {
    auto metrics = EqualityMetrics(5);
    auto params = MakeParams(kIris, 1.0, 0.8, 30, 3, metrics);
    params[kRfdCrossoverProbability] = 0.0;
    params[kRfdMutationProbability] = 0.0;
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ExpectValidRfds(rfds, 0.8);
}

}  // namespace tests
