#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/gfd/gfd_miner.h"
#include "config/names.h"
#include "csv_config_util.h"

using namespace algos;
using algos::StdParamsMap;

namespace tests {

class GfdMiningTest : public ::testing::Test {
public:
    std::filesystem::path const current_path = kTestDataDir / "graph_data";

    static std::unique_ptr<algos::GfdMiner> CreateGfdMiningInstance(
            std::filesystem::path const& graph_path, std::size_t const k, std::size_t const sigma) {
        StdParamsMap option_map = {{config::names::kGraphData, graph_path},
                                   {config::names::kGfdK, k},
                                   {config::names::kGfdSigma, sigma}};
        return algos::CreateAndLoadAlgorithm<GfdMiner>(option_map);
    }
};

TEST_F(GfdMiningTest, TestMinGfd) {
    std::filesystem::path const gfd_path = GfdMiningTest::current_path / "blogs_gfd.dot";
    std::ifstream f(gfd_path);
    Gfd gfd = parser::graph_parser::ReadGfd(f);
    std::vector<Gfd> gfds = {gfd};

    std::filesystem::path const graph_path = GfdMiningTest::current_path / "blogs_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 2, 3);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    algorithm = CreateGfdMiningInstance(graph_path, 3, 3);
    algorithm->Execute();
    std::size_t expected_size = gfd_list.size();
    gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

TEST_F(GfdMiningTest, TestComplexConclusion) {
    std::filesystem::path const gfd_path = GfdMiningTest::current_path / "channels_gfd.dot";
    std::ifstream f(gfd_path);
    Gfd gfd = parser::graph_parser::ReadGfd(f);
    std::vector<Gfd> gfds = {gfd};

    std::filesystem::path const graph_path = GfdMiningTest::current_path / "channels_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 2, 3);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 1;
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

TEST_F(GfdMiningTest, TestMovies) {
    std::filesystem::path const graph_path = GfdMiningTest::current_path / "movies_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 4, 2);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    std::size_t expected_size = 0;
    ASSERT_EQ(expected_size, gfd_list.size());
}

TEST_F(GfdMiningTest, TestSymbols) {
    std::vector<Gfd> gfds = {};
    std::size_t expected_size = 2;
    for (std::size_t index = 0; index < expected_size; ++index) {
        std::filesystem::path const gfd_path =
                GfdMiningTest::current_path / ("symbols_gfd" + std::to_string(index + 1) + ".dot");
        std::ifstream f(gfd_path);
        Gfd gfd = parser::graph_parser::ReadGfd(f);
        gfds.push_back(gfd);
    }
    std::filesystem::path const graph_path = GfdMiningTest::current_path / "symbols_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 2, 5);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

TEST_F(GfdMiningTest, TestShapes) {
    std::vector<Gfd> gfds = {};
    std::size_t expected_size = 2;
    for (std::size_t index = 0; index < expected_size; ++index) {
        std::filesystem::path const gfd_path =
                GfdMiningTest::current_path / ("shapes_gfd" + std::to_string(index + 1) + ".dot");
        std::ifstream f(gfd_path);
        Gfd gfd = parser::graph_parser::ReadGfd(f);
        gfds.push_back(gfd);
    }
    std::filesystem::path const graph_path = GfdMiningTest::current_path / "shapes_graph.dot";
    auto algorithm = CreateGfdMiningInstance(graph_path, 3, 10);
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
    for (std::size_t index = 0; index < gfd_list.size(); ++index) {
        ASSERT_TRUE(gfds.at(index) == gfd_list.at(index));
    }
}

}  // namespace tests
