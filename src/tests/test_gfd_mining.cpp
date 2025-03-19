#include <cstdlib>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/gfd/gfd_miner.h"
#include "all_paths.h"
#include "config/names.h"

using namespace algos;
using algos::StdParamsMap;

namespace tests {

class GfdMiningTest : public ::testing::Test {
public:
    struct TestConfig {
        std::size_t k;
        std::size_t sigma;
    };

    static std::unique_ptr<algos::GfdMiner> CreateGfdMiningInstance(
            std::filesystem::path const& graph_path, TestConfig const& gfdConfig) {
        StdParamsMap option_map = {{config::names::kGraphData, graph_path},
                                   {config::names::kGfdK, gfdConfig.k},
                                   {config::names::kGfdSigma, gfdConfig.sigma}};
        return algos::CreateAndLoadAlgorithm<GfdMiner>(option_map);
    }

    static void ExecuteAndCompare(std::unique_ptr<algos::GfdMiner> const& algorithm,
                                  std::vector<Gfd> const& expected_gfds) {
        algorithm->Execute();
        auto const gfd_list = algorithm->GfdList();
        ASSERT_EQ(expected_gfds.size(), gfd_list.size());
        ASSERT_THAT(gfd_list, ::testing::ElementsAreArray(expected_gfds));
    }
};

TEST_F(GfdMiningTest, TestMinGfd) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestBlogsGfd)};

    std::filesystem::path const graph_path = kGfdTestBlogsGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 2, .sigma = 3});
    ExecuteAndCompare(algorithm, gfds);
    algorithm = CreateGfdMiningInstance(graph_path, {.k = 3, .sigma = 3});
    ExecuteAndCompare(algorithm, gfds);
}

TEST_F(GfdMiningTest, TestComplexConclusion) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestChannelsGfd)};

    std::filesystem::path const graph_path = kGfdTestChannelsGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 2, .sigma = 3});
    ExecuteAndCompare(algorithm, gfds);
}

TEST_F(GfdMiningTest, TestMovies) {
    std::filesystem::path const graph_path = kGfdTestMoviesGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 4, .sigma = 2});
    ExecuteAndCompare(algorithm, {});
}

TEST_F(GfdMiningTest, TestSymbols) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestSymbolsGfd1), MakeGfd(kGfdTestSymbolsGfd2)};

    std::filesystem::path const graph_path = kGfdTestSymbolsGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 2, .sigma = 5});
    ExecuteAndCompare(algorithm, gfds);
}

TEST_F(GfdMiningTest, TestShapes) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestShapesGfd1), MakeGfd(kGfdTestShapesGfd2)};

    std::filesystem::path const graph_path = kGfdTestShapesGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 3, .sigma = 10});
    ExecuteAndCompare(algorithm, gfds);
}

}  // namespace tests
