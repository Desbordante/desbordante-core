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

}
