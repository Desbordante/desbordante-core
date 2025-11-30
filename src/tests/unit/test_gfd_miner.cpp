#include <cstdlib>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/gfd/gfd_miner/gfd_miner.h"
#include "core/config/names.h"
#include "tests/unit/all_gfd_paths.h"

namespace gfd {
void PrintTo(model::Gfd const& gfd, std::ostream* os) {
    *os << gfd.ToString();
}
}  // namespace gfd

namespace tests {

struct GfdMinerTestParams {
private:
public:
    struct TestConfig {
        std::size_t k;
        std::size_t sigma;
    };

    algos::StdParamsMap params;
    std::vector<std::filesystem::path> gfd_paths;

    GfdMinerTestParams(std::filesystem::path const& graph_path, TestConfig const& test_config,
                       std::vector<std::filesystem::path> const& expected_gfd_paths)
        : params({{config::names::kGraphData, graph_path},
                  {config::names::kGfdK, test_config.k},
                  {config::names::kGfdSigma, test_config.sigma}}),
          gfd_paths(expected_gfd_paths) {}
};

class GfdMinerTest : public ::testing::TestWithParam<GfdMinerTestParams> {
protected:
    static std::vector<model::Gfd> GetExpectedGfds() {
        std::vector<model::Gfd> gfds;
        for (auto const& path : GetParam().gfd_paths) {
            gfds.push_back(MakeGfd(path));
        }
        return gfds;
    }
};

TEST_P(GfdMinerTest, CompareResultTest) {
    auto algorithm = algos::CreateAndLoadAlgorithm<algos::GfdMiner>(GetParam().params);
    algorithm->Execute();
    ASSERT_THAT(algorithm->GfdList(), ::testing::ElementsAreArray(GetExpectedGfds()));
}

INSTANTIATE_TEST_SUITE_P(
        GfdMinerTestSuite, GfdMinerTest,
        ::testing::Values(
                GfdMinerTestParams(kGfdTestBlogsGraph, {.k = 2, .sigma = 3}, {kGfdTestBlogsGfd}),
                GfdMinerTestParams(kGfdTestBlogsGraph, {.k = 3, .sigma = 3}, {kGfdTestBlogsGfd}),
                GfdMinerTestParams(kGfdTestChannelsGraph, {.k = 2, .sigma = 3},
                                   {kGfdTestChannelsGfd}),
                GfdMinerTestParams(kGfdTestMoviesGraph, {.k = 4, .sigma = 2}, {}),
                GfdMinerTestParams(kGfdTestSymbolsGraph, {.k = 2, .sigma = 5},
                                   {kGfdTestSymbolsGfd1, kGfdTestSymbolsGfd2}),
                GfdMinerTestParams(kGfdTestShapesGraph, {.k = 3, .sigma = 10},
                                   {kGfdTestShapesGfd1, kGfdTestShapesGfd2})));

}  // namespace tests
