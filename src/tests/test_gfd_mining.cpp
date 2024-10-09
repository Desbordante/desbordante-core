#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/gfd/gfd_miner.h"
#include "config/names.h"
#include "csv_config_util.h"

using namespace algos;
using algos::StdParamsMap;

namespace tests {

std::filesystem::path const current_path = kTestDataDir / "graph_data";

class GfdMiningTest : public ::testing::Test {
public:
    static std::unique_ptr<algos::GfdMiner> CreateGfdMiningInstance(
            std::filesystem::path const& graph_path, std::size_t const k, std::size_t const sigma) {
        StdParamsMap option_map = {{config::names::kGraphData, graph_path},
                                   {config::names::kGfdK, k},
                                   {config::names::kGfdSigma, sigma}};
        return algos::CreateAndLoadAlgorithm<GfdMiner>(option_map);
    }
};

TEST_F(GfdMiningTest, TestMinGfd) {
    std::filesystem::path const graph_path = current_path / "blogs_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 2, 3);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    algorithm = CreateGfdMiningInstance(graph_path, 3, 3);
    algorithm->Execute();
    std::size_t expected_size = gfd_list.size();
    gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
}

TEST_F(GfdMiningTest, TestComplexConclusion) {
    std::filesystem::path const graph_path = current_path / "channels_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 2, 3);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 1;
    ASSERT_EQ(expected_size, gfd_list.size());
}

TEST_F(GfdMiningTest, TestMovies) {
    std::filesystem::path const graph_path = current_path / "movies_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 4, 2);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 0;
    ASSERT_EQ(expected_size, gfd_list.size());
}

TEST_F(GfdMiningTest, TestSymbols) {
    std::filesystem::path const graph_path = current_path / "symbols_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 2, 5);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 2;
    ASSERT_EQ(expected_size, gfd_list.size());
}

TEST_F(GfdMiningTest, TestShapes) {
    std::filesystem::path const graph_path = current_path / "shapes_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 3, 10);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 2;
    ASSERT_EQ(expected_size, gfd_list.size());
}

}  // namespace tests
