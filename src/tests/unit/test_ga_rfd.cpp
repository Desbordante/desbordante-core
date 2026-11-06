#include <algorithm>
#include <bit>
#include <cstdint>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
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
        EXPECT_FALSE(rfd.lhs_indices.empty()) << "LHS must not be empty";
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

    config::CustomMetricsType ga_metrics;
    for (int i = 0; i < 4; ++i) ga_metrics.push_back(kAbsDiff);
    ga_metrics.push_back(kEq);

    std::vector<double> sim_vec{min_similarity};

    algos::StdParamsMap params{{kTable, table},
                               {kCustomMetrics, ga_metrics},
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

    config::CustomMetricsType metrics(ncols, kEq);
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
                                           RFDTestParams{kBreastCancer, 1.0, 0.9, 100, 10, true, 1},
                                           RFDTestParams{kNeighbors10k, 0.5, 0.9, 10, 3, true, 1},
                                           RFDTestParams{kTestFD, 1.0, 0.9, 40, 5, true, 1},
                                           RFDTestParams{kTestLong, 1.0, 0.9, 30, 5, true, 1},
                                           RFDTestParams{kTestWide, 1.0, 0.9, 30, 5, true, 1},
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
        for (std::size_t i = 0; i < rfd.lhs_indices.size(); ++i) {
            EXPECT_LT(rfd.lhs_indices[i], 5u);
        }
        EXPECT_LT(rfd.rhs_index, 5u);
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

static std::size_t NumberOfColumnsIn(CSVConfig const& csv_config) {
    return std::make_shared<CSVParser>(csv_config)->GetNumberOfColumns();
}

static std::size_t BruteForceSupport(std::vector<std::vector<std::string>> const& rows,
                                     std::vector<std::size_t> const& attrs) {
    std::size_t support = 0;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        for (std::size_t j = i + 1; j < rows.size(); ++j) {
            bool match = true;
            for (std::size_t a : attrs) match = match && (rows[i][a] == rows[j][a]);
            if (match) ++support;
        }
    }
    return support;
}

// Exact (support, confidence) of every valid (lhs, rhs) candidate on the
// given rows, equating cells pairwise, which is what the equality metric with
// min_similarity = 1.0 yields. A rule with no similar LHS pair holds
// vacuously, so its confidence is 1.0 like the algorithm reports
struct BruteForceInterpretation {
    std::size_t total_pairs;
    std::unordered_map<std::uint64_t, double> confidence;
    std::unordered_map<std::uint64_t, double> support;
};

static std::uint64_t CandidateKey(AttributeMask lhs_mask, std::size_t rhs) {
    return lhs_mask | (std::uint64_t{1} << (rhs + 32));
}

static double PairwiseConfidence(std::vector<std::vector<std::string>> const& rows,
                                 AttributeMask lhs_mask, std::size_t rhs) {
    std::size_t count_lhs = 0;
    std::size_t count_both = 0;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        for (std::size_t j = i + 1; j < rows.size(); ++j) {
            bool lhs_match = true;
            bool both_match = true;
            for (std::size_t attribute = 0; attribute < rows[i].size(); ++attribute) {
                bool equal = rows[i][attribute] == rows[j][attribute];
                if ((lhs_mask & (1u << attribute)) != 0) lhs_match = lhs_match && equal;
                if (attribute == rhs) both_match = both_match && equal;
            }
            if (lhs_match) ++count_lhs;
            if (lhs_match && both_match) ++count_both;
        }
    }
    if (count_lhs == 0) return 1.0;
    return static_cast<double>(count_both) / count_lhs;
}

static BruteForceInterpretation BruteForceEvaluate(
        std::vector<std::vector<std::string>> const& rows) {
    BruteForceInterpretation result;
    result.total_pairs = rows.size() * (rows.size() - 1) / 2;
    std::size_t const kAttributes = rows.front().size();
    for (std::size_t rhs = 0; rhs < kAttributes; ++rhs) {
        for (AttributeMask lhs_mask = 1; lhs_mask < (1u << kAttributes); ++lhs_mask) {
            if ((lhs_mask & (1u << rhs)) != 0) continue;
            std::uint64_t const key = CandidateKey(lhs_mask, rhs);
            std::vector<std::size_t> lhs_indices;
            for (std::size_t attr = 0; attr < kAttributes; ++attr) {
                if ((lhs_mask & (1u << attr)) != 0) lhs_indices.push_back(attr);
            }
            std::size_t const count_lhs = BruteForceSupport(rows, lhs_indices);
            result.support[key] = static_cast<double>(count_lhs) / result.total_pairs;
            result.confidence[key] = PairwiseConfidence(rows, lhs_mask, rhs);
        }
    }
    return result;
}

// GA-RFD supports at most 31 attributes (AttributeMask is 32-bit, 1 bit is
// reserved for the rhs).
TEST(GARfdEdge, MoreThan31AttributesThrows) {
    std::size_t const ncols = NumberOfColumnsIn(kRfd32Attrs);
    ASSERT_EQ(ncols, std::size_t{32});
    auto params = MakeParams(kRfd32Attrs, std::vector<double>(ncols, 1.0), 0.5, 10, 1,
                             EqualityMetrics(ncols));
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<GaRfd>(params), std::exception);
}

// A table with a single row has no tuple pairs to compare, so it must be
// rejected at load time.
TEST(GARfdEdge, SingleRowTableThrows) {
    std::size_t const ncols = NumberOfColumnsIn(kRfdSingleRow);
    ASSERT_EQ(ncols, std::size_t{2});
    auto params = MakeParams(kRfdSingleRow, std::vector<double>(ncols, 1.0), 0.5, 10, 1,
                             EqualityMetrics(ncols));
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<GaRfd>(params), std::exception);
}

// Pairwise matching ignores NULL/empty values entirely: on
// rfd_mixed_nulls.csv column 0 holds (1, hello, NULL, NULL), so no pair ever
// matches on it. Therefore a rule whose LHS includes that attribute has zero
// support and holds vacuously (confidence 1), and a rule with it as the RHS
// can never be satisfied on matching pairs (confidence 0).
TEST(GARfdNulls, NullsAndEmptiesNeverMatch) {
    auto params = MakeParams(kRfdMixedNulls, std::vector<double>{1.0}, 0.0, 30, 10,
                             EqualityMetrics(NumberOfColumnsIn(kRfdMixedNulls)));
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ASSERT_FALSE(rfds.empty());
    ExpectValidRfds(rfds, 0.0);
    constexpr std::size_t kNullColumn = 0;
    for (auto const& rfd : rfds) {
        bool const null_in_lhs = std::find(rfd.lhs_indices.begin(), rfd.lhs_indices.end(),
                                           kNullColumn) != rfd.lhs_indices.end();
        if (null_in_lhs) {
            EXPECT_DOUBLE_EQ(rfd.support, 0.0) << rfd.ToString();
            EXPECT_DOUBLE_EQ(rfd.confidence, 1.0) << rfd.ToString();
        } else if (rfd.rhs_index == kNullColumn) {
            EXPECT_DOUBLE_EQ(rfd.confidence, 0.0) << rfd.ToString();
        }
    }
}

// minconf = 1.0 with the equality metric must mine exact functional
// dependencies. Column id is all-unique, so no pair matches on it: the rules
// it determines hold vacuously and must still be reported (support 0,
// confidence 1), not dropped.
TEST(GARfdExactFD, UniqueKeyColumnMinedWithMinConfOne) {
    auto params = MakeParams(kRfdUniqueKey, 1.0, 1.0, 30, 10, EqualityMetrics(3));
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ASSERT_FALSE(rfds.empty());
    for (auto const& rfd : rfds) {
        EXPECT_DOUBLE_EQ(rfd.confidence, 1.0) << rfd.ToString();
    }
    bool id_lhs_a = false;
    bool id_lhs_b = false;
    for (auto const& rfd : rfds) {
        if (rfd.lhs_indices == std::vector<std::size_t>{0} && rfd.rhs_index == 1) {
            id_lhs_a = true;
            EXPECT_DOUBLE_EQ(rfd.support, 0.0) << rfd.ToString();
        }
        if (rfd.lhs_indices == std::vector<std::size_t>{0} && rfd.rhs_index == 2) {
            id_lhs_b = true;
            EXPECT_DOUBLE_EQ(rfd.support, 0.0) << rfd.ToString();
        }
    }
    EXPECT_TRUE(id_lhs_a) << "{id} -> a must be mined";
    EXPECT_TRUE(id_lhs_b) << "{id} -> b must be mined";
}

// Classic exact FD mining: a determines b on this table, so with
// minconf = 1.0 both directions that hold on every matching pair must be
// reported with confidence exactly 1.
TEST(GARfdExactFD, ClassicExactFDMined) {
    auto params = MakeParams(kRfdExactFD, 1.0, 1.0, 30, 10, EqualityMetrics(2));
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ASSERT_FALSE(rfds.empty());
    for (auto const& rfd : rfds) {
        EXPECT_DOUBLE_EQ(rfd.confidence, 1.0) << rfd.ToString();
    }
    bool a_to_b = false;
    bool b_to_a = false;
    for (auto const& rfd : rfds) {
        if (rfd.lhs_indices == std::vector<std::size_t>{0} && rfd.rhs_index == 1) {
            a_to_b = true;
            EXPECT_DOUBLE_EQ(rfd.support, 1.0 / 3.0) << rfd.ToString();
        }
        if (rfd.lhs_indices == std::vector<std::size_t>{1} && rfd.rhs_index == 0) {
            b_to_a = true;
        }
    }
    EXPECT_TRUE(a_to_b) << "{a} -> b must be mined";
    EXPECT_TRUE(b_to_a) << "{b} -> a must be mined";
}

// Reveals the result confidence and support against exhaustive enumeration so
// that a run producing a single arbitrary RFD cannot pass. Note that
// confidence must use both/lhs directly: computing (both/total)/(lhs/total)
// would yield 0.7999... for an exact 0.8 dependency and drop it at the
// minconf boundary
TEST(GARfdSupport, ReportedValuesMatchBruteForce) {
    auto params =
            MakeParams(kRfdDuplicates, std::vector<double>(5, 1.0), 0.0, 30, 5, EqualityMetrics(5));
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    ASSERT_FALSE(rfds.empty());

    auto parser = std::make_shared<CSVParser>(kRfdDuplicates);
    std::vector<std::vector<std::string>> rows;
    while (parser->HasNextRow()) rows.push_back(parser->GetNextRow());
    ASSERT_EQ(rows.size(), 15u);
    std::size_t const total_pairs = rows.size() * (rows.size() - 1) / 2;

    for (auto const& rfd : rfds) {
        std::size_t const support_lhs = BruteForceSupport(rows, rfd.lhs_indices);
        std::vector<std::size_t> both = rfd.lhs_indices;
        both.push_back(rfd.rhs_index);
        std::size_t const support_both = BruteForceSupport(rows, both);
        EXPECT_DOUBLE_EQ(rfd.support, static_cast<double>(support_lhs) / total_pairs)
                << rfd.ToString();
        if (support_lhs == 0) {
            EXPECT_DOUBLE_EQ(rfd.confidence, 1.0) << rfd.ToString();
        } else {
            EXPECT_DOUBLE_EQ(rfd.confidence, static_cast<double>(support_both) / support_lhs)
                    << rfd.ToString();
        }
    }
}

// Reviewed-borderline case: with 6 pairs, 5 similar on the LHS and 4 of them
// also similar on the RHS, the confidence is exactly 0.8. Computing it as
// (4/6)/(5/6) would give 0.7999999999999999 and silently drop this dependency
// with minconf = 0.8. Built-in abs_diff measures |x-y|/max(|x|,|y|), so the
// fixture uses values whose relative distances are dyadic and hit the exact
// 0.5 boundary for min_similarity = 0.5
TEST(GARfdConfidence, ExactMinConfidenceBorderNotLostToRounding) {
    config::CustomMetricsType metrics{kAbsDiff, kAbsDiff};
    auto params = MakeParams(kRfdConfEdge, std::vector<double>{0.5, 0.5}, 0.8, 30, 10, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    bool a_to_b = false;
    bool b_to_a = false;
    for (auto const& rfd : rfds) {
        if (rfd.lhs_indices == std::vector<std::size_t>{0} && rfd.rhs_index == 1) {
            a_to_b = true;
            EXPECT_DOUBLE_EQ(rfd.confidence, 0.8) << rfd.ToString();
            EXPECT_DOUBLE_EQ(rfd.support, 5.0 / 6.0) << rfd.ToString();
        }
        if (rfd.lhs_indices == std::vector<std::size_t>{1} && rfd.rhs_index == 0) {
            b_to_a = true;
            EXPECT_DOUBLE_EQ(rfd.confidence, 1.0) << rfd.ToString();
        }
    }
    EXPECT_TRUE(a_to_b) << "{a} -> b with confidence exactly 0.8 must be mined";
    EXPECT_TRUE(b_to_a) << "{b} -> a must be mined";
}

// Pairs whose similarity is exactly min_similarity must count as similar:
// for values 90.0 and 100.0 the relative difference is 0.1000000000000000055,
// and 0.1000000000000000055 + 0.9 rounds to exactly 1.0 in double
// arithmetic, while an explicit 1.0 - 0.9 threshold gives
// 0.09999999999999998 and loses the pair
TEST(GARfdMinSimilarity, ExactMinSimilarityBorderNotLostToRounding) {
    config::CustomMetricsType metrics{kAbsDiff, kAbsDiff};
    auto params = MakeParams(kRfdSimEdge, std::vector<double>{0.9, 0.9}, 0.9, 30, 10, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    bool a_to_b = false;
    bool b_to_a = false;
    for (auto const& rfd : rfds) {
        EXPECT_DOUBLE_EQ(rfd.confidence, 1.0) << rfd.ToString();
        if (rfd.lhs_indices == std::vector<std::size_t>{0} && rfd.rhs_index == 1) {
            a_to_b = true;
            EXPECT_DOUBLE_EQ(rfd.support, 1.0) << rfd.ToString();
        }
        if (rfd.lhs_indices == std::vector<std::size_t>{1} && rfd.rhs_index == 0) {
            b_to_a = true;
            EXPECT_DOUBLE_EQ(rfd.support, 1.0) << rfd.ToString();
        }
    }
    EXPECT_TRUE(a_to_b) << "the pair with similarity exactly 0.9 must match as the LHS support";
    EXPECT_TRUE(b_to_a) << "the pair with similarity exactly 0.9 must match as the LHS support";
}

// Completeness: with minconf = 1.0 the algorithm is expected to cover every
// exact functional dependency of a small table, and the reported set must be
// exactly the brute-force set of dependencies (including vacuous rules
// determined by key columns)
TEST(GARfdBruteForce, MinConfOneReportsExactlyAllExactFDs) {
    auto parser = std::make_shared<CSVParser>(kRfdBruteSmall);
    std::vector<std::vector<std::string>> rows;
    while (parser->HasNextRow()) rows.push_back(parser->GetNextRow());
    ASSERT_EQ(rows.size(), 6u);
    auto brute = BruteForceEvaluate(rows);

    auto params = MakeParams(kRfdBruteSmall, 1.0, 1.0, 200, 10, EqualityMetrics(5));
    auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
    algo->Execute();
    auto rfds = algo->GetRfds();
    for (auto const& rfd : rfds) {
        AttributeMask mask = 0;
        for (std::size_t index : rfd.lhs_indices) mask |= 1u << index;
        auto const key = CandidateKey(mask, rfd.rhs_index);
        ASSERT_TRUE(brute.confidence.count(key))
                << "the algorithm reported an invalid rule " << rfd.ToString();
        EXPECT_DOUBLE_EQ(rfd.confidence, brute.confidence.at(key)) << rfd.ToString();
        EXPECT_DOUBLE_EQ(rfd.support, brute.support.at(key)) << rfd.ToString();
    }
    for (auto const& [key, confidence] : brute.confidence) {
        if (confidence < 1.0) continue;
        AttributeMask const lhs_mask = static_cast<AttributeMask>(key & 0xFFFFFFFF);
        // the upper half of the key stores 1 << rhs, so the index is its
        // position of the set bit, not the shifted value itself
        std::size_t const rhs = static_cast<std::size_t>(std::countr_zero(key >> 32));
        bool found = false;
        for (auto const& rfd : rfds) {
            AttributeMask mask = 0;
            for (std::size_t index : rfd.lhs_indices) mask |= 1u << index;
            if (mask == lhs_mask && rfd.rhs_index == rhs) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "the exact FD with lhs mask " << lhs_mask << " and rhs " << rhs
                           << " was missed";
    }
}

}  // namespace tests
