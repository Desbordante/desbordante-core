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
};

TEST_F(GfdMiningTest, TestMinGfd) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestBlogsGfd)};

    std::filesystem::path const graph_path = kGfdTestBlogsGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 2, .sigma = 3});
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    algorithm = CreateGfdMiningInstance(graph_path, {.k = 3, .sigma = 3});
    algorithm->Execute();
    std::size_t expected_size = gfd_list.size();
    gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

TEST_F(GfdMiningTest, TestComplexConclusion) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestChannelsGfd)};

    std::filesystem::path const graph_path = kGfdTestChannelsGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 2, .sigma = 3});
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 1;
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

TEST_F(GfdMiningTest, TestMovies) {
    std::filesystem::path const graph_path = kGfdTestMoviesGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 4, .sigma = 2});
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 0;
    ASSERT_EQ(expected_size, gfd_list.size());
}

TEST_F(GfdMiningTest, TestSymbols) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestSymbolsGfd1), MakeGfd(kGfdTestSymbolsGfd2)};
    std::size_t expected_size = gfds.size();

    std::filesystem::path const graph_path = kGfdTestSymbolsGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 2, .sigma = 5});
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

TEST_F(GfdMiningTest, TestShapes) {
    std::vector<Gfd> gfds = {MakeGfd(kGfdTestShapesGfd1), MakeGfd(kGfdTestShapesGfd2)};
    std::size_t expected_size = gfds.size();

    std::filesystem::path const graph_path = kGfdTestShapesGraph;
    std::unique_ptr<algos::GfdMiner> algorithm =
            CreateGfdMiningInstance(graph_path, {.k = 3, .sigma = 10});
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

}  // namespace tests
