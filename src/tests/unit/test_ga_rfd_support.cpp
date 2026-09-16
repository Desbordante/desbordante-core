#include <memory>
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
namespace rfd = algos::rfd;
namespace util = ::util;

class GaRfdTester {
public:
    static void BuildMatchBitsets(rfd::GaRfd& algo) {
        algo.BuildMatchBitsets();
    }

    static std::size_t ComputeSupport(rfd::GaRfd const& algo, uint32_t mask) {
        return algo.ComputeSupport(mask);
    }
};

static algos::StdParamsMap MakeParams(config::InputTable const& table,
                                      std::vector<double> const& min_sim, double beta,
                                      std::size_t pop_size, std::size_t max_gen,
                                      config::CustomMetricsType metrics = {}) {
    algos::StdParamsMap params{{config::names::kTable, table},
                               {config::names::kRfdMinSimilarity, min_sim},
                               {config::names::kRfdMinimumConfidence, beta},
                               {config::names::kPopulationSize, pop_size},
                               {config::names::kRfdMaxGenerations, max_gen},
                               {config::names::kRfdCrossoverProbability, 0.85},
                               {config::names::kRfdMutationProbability, 0.3},
                               {config::names::kSeed, std::uint32_t{42}}};
    if (!metrics.empty()) {
        params["metrics"] = metrics;
    }
    return params;
}

TEST(GARfdSupport, SupportComputationOnIris) {
    config::InputTable table = std::make_shared<CSVParser>(kIris);

    config::CustomMetricsType metrics(5);
    for (int i = 0; i < 5; ++i) metrics[i] = rfd::EqualityMetric();

    std::vector<double> sim_vec(5, 1.0);

    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();

    constexpr std::size_t total_pairs = 150 * 149 / 2;

    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, 0), total_pairs);

    uint32_t species_mask = 1u << 4;
    constexpr std::size_t expected_species_support = 3 * (50 * 49 / 2);
    std::size_t actual_species = GaRfdTester::ComputeSupport(*algo, species_mask);
    EXPECT_EQ(actual_species, expected_species_support)
            << "Support for species should be 3675 (3 classes of 50)";

    uint32_t all_attrs_mask = (1u << 5) - 1;
    std::size_t all_support = GaRfdTester::ComputeSupport(*algo, all_attrs_mask);
    EXPECT_GT(all_support, 0) << "There are duplicate rows in this Iris version";

    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, species_mask), expected_species_support);
}

TEST(GARfdSupport, CacheReuse) {
    config::InputTable table = std::make_shared<CSVParser>(kIris);
    config::CustomMetricsType metrics(5, rfd::EqualityMetric());
    std::vector<double> sim_vec(5, 1.0);

    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    GaRfdTester::BuildMatchBitsets(*algo);

    uint32_t mask = 1u << 4;
    std::size_t first = GaRfdTester::ComputeSupport(*algo, mask);
    std::size_t second = GaRfdTester::ComputeSupport(*algo, mask);
    EXPECT_EQ(first, second);
}

}  // namespace tests
