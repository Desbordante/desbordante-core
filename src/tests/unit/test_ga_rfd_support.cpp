#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/distance_metric.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/config/names.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "core/util/custom_metric/custom_metric.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
using namespace algos::rfd;
using namespace config::names;
namespace util = ::util;

static algos::StdParamsMap MakeParams(config::InputTable const& table,
                                      std::vector<double> const& min_sim, double min_confidence,
                                      std::size_t pop_size, std::size_t max_gen,
                                      config::CustomMetricsType metrics = {}) {
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

TEST(GARfdSupport, MixedPlaceholdersNeverMatch) {
    auto run_and_collect = [](std::shared_ptr<util::ICustomMetric> column_a_metric,
                              double min_sim) {
        config::InputTable table = std::make_shared<CSVParser>(kRfdMixedNulls);
        config::CustomMetricsType metrics{column_a_metric, EqualityMetric()};
        std::vector<double> sim_vec{min_sim, 1.0};

        auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
        auto algo = algos::CreateAndLoadAlgorithm<GaRfd>(params);
        algo->Execute();
        return algo->GetRfds();
    };

    for (auto const& [metric, min_sim] : {std::pair{LevenshteinMetric(), 1.0},
                                          {AbsoluteDifferenceMetric(), 0.9},
                                          {AbsoluteThresholdMetricFactory(10.0), 1.0}}) {
        for (auto const& rfd : run_and_collect(metric, min_sim)) {
            EXPECT_TRUE(std::find(rfd.lhs.begin(), rfd.lhs.end(), "a") == rfd.lhs.end())
                    << rfd.ToString();
        }
    }
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

TEST(GARfdSupport, ReportedSupportsAreExact) {
    config::InputTable table = std::make_shared<CSVParser>(kRfdDuplicates);
    config::CustomMetricsType metrics(5, EqualityMetric());
    std::vector<double> sim_vec(5, 1.0);

    auto params = MakeParams(table, sim_vec, 0.0, 30, 5, metrics);
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
        if (support_lhs == 0) {
            EXPECT_DOUBLE_EQ(rfd.support, 0.0);
            EXPECT_DOUBLE_EQ(rfd.confidence, 0.0);
            continue;
        }
        std::vector<std::size_t> both = rfd.lhs_indices;
        both.push_back(rfd.rhs_index);
        std::size_t const support_both = BruteForceSupport(rows, both);
        EXPECT_DOUBLE_EQ(rfd.support, static_cast<double>(support_lhs) / total_pairs);
        EXPECT_DOUBLE_EQ(rfd.confidence, static_cast<double>(support_both) / support_lhs);
    }
}

}  // namespace tests
