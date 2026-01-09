#include <climits>
#include <filesystem>
#include <map>
#include <sstream>
#include <unordered_set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fsm/gspan/dfscode.h"
#include "core/algorithms/fsm/gspan/extended_edge.h"
#include "core/algorithms/fsm/gspan/graph_parser.h"
#include "core/algorithms/fsm/gspan/gspan.h"
#include "core/config/names.h"
#include "tests/common/csv_config_util.h"

namespace tests {

namespace {

std::filesystem::path const kGraphDataDir = kTestDataDir / "graph_data";

std::filesystem::path const kGSpanTestSimple = kGraphDataDir / "gspan_test_simple.txt";
std::filesystem::path const kGSpanTestTriangle = kGraphDataDir / "gspan_test_triangle.txt";
std::filesystem::path const kGSpanTestSingleVertex = kGraphDataDir / "gspan_test_single_vertex.txt";
std::filesystem::path const kGSpanTestChain = kGraphDataDir / "gspan_test_chain.txt";
std::filesystem::path const kGSpanTestEmpty = kGraphDataDir / "gspan_test_empty.txt";
std::filesystem::path const kGSpanLargeGraph = kGraphDataDir / "gspan_mutag_graph.txt";

algos::StdParamsMap CreateGSpanParams(std::filesystem::path const& graph_path, double min_support,
                                      bool output_single_vertices = true, int max_edges = INT_MAX,
                                      bool output_graph_ids = true) {
    using namespace config::names;
    return {{kGraphDatabase, graph_path},
            {kGSpanMinimumSupport, min_support},
            {kOutputSingleVertices, output_single_vertices},
            {kMaxNumberOfEdges, max_edges},
            {kOutputGraphIds, output_graph_ids}};
}

}  // namespace

class DFSCodeTest : public ::testing::Test {};

TEST_F(DFSCodeTest, EmptyCode) {
    gspan::DFSCode code;
    EXPECT_TRUE(code.Empty());
    EXPECT_EQ(code.Size(), 0);
    EXPECT_EQ(code.GetRightMost(), -1);
}

TEST_F(DFSCodeTest, AddSingleEdge) {
    gspan::DFSCode code;
    gspan::ExtendedEdge edge(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1);
    code.Add(edge);

    EXPECT_FALSE(code.Empty());
    EXPECT_EQ(code.Size(), 1);
    EXPECT_EQ(code.GetRightMost(), 1);
}

TEST_F(DFSCodeTest, AddMultipleEdges) {
    gspan::DFSCode code;
    code.Add(gspan::ExtendedEdge(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1));
    code.Add(gspan::ExtendedEdge(gspan::Vertex{1, 2}, gspan::Vertex{2, 3}, 1));

    EXPECT_EQ(code.Size(), 2);
    EXPECT_EQ(code.GetRightMost(), 2);
}

TEST_F(DFSCodeTest, RightMostPath) {
    gspan::DFSCode code;
    code.Add(gspan::ExtendedEdge(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1));
    code.Add(gspan::ExtendedEdge(gspan::Vertex{1, 2}, gspan::Vertex{2, 3}, 1));

    auto const& path = code.GetRightMostPath();
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(code.OnRightMostPath(0));
    EXPECT_TRUE(code.OnRightMostPath(1));
    EXPECT_TRUE(code.OnRightMostPath(2));
}

TEST_F(DFSCodeTest, ContainEdge) {
    gspan::DFSCode code;
    code.Add(gspan::ExtendedEdge(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1));
    code.Add(gspan::ExtendedEdge(gspan::Vertex{1, 2}, gspan::Vertex{2, 3}, 1));

    EXPECT_TRUE(code.ContainEdge(0, 1));
    EXPECT_TRUE(code.ContainEdge(1, 0));
    EXPECT_TRUE(code.ContainEdge(1, 2));
    EXPECT_FALSE(code.ContainEdge(0, 2));
    EXPECT_FALSE(code.ContainEdge(3, 4));
}

TEST_F(DFSCodeTest, GetAllLabels) {
    gspan::DFSCode code;
    code.Add(gspan::ExtendedEdge(gspan::Vertex{0, 10}, gspan::Vertex{1, 20}, 1));
    code.Add(gspan::ExtendedEdge(gspan::Vertex{1, 20}, gspan::Vertex{2, 30}, 1));

    auto labels = code.GetAllLabels();
    ASSERT_EQ(labels.size(), 3);
    EXPECT_EQ(labels[0], 10);
    EXPECT_EQ(labels[1], 20);
    EXPECT_EQ(labels[2], 30);
}

TEST_F(DFSCodeTest, OperatorIndex) {
    gspan::DFSCode code;
    gspan::ExtendedEdge edge1(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 5);
    gspan::ExtendedEdge edge2(gspan::Vertex{1, 2}, gspan::Vertex{2, 3}, 6);
    code.Add(edge1);
    code.Add(edge2);

    EXPECT_EQ(code[0].label, 5);
    EXPECT_EQ(code[1].label, 6);
}

class ExtendedEdgeTest : public ::testing::Test {};

TEST_F(ExtendedEdgeTest, ParameterizedConstructor) {
    gspan::Vertex v1{0, 1};
    gspan::Vertex v2{1, 2};
    gspan::ExtendedEdge edge(v1, v2, 5);

    EXPECT_EQ(edge.vertex1.id, 0);
    EXPECT_EQ(edge.vertex1.label, 1);
    EXPECT_EQ(edge.vertex2.id, 1);
    EXPECT_EQ(edge.vertex2.label, 2);
    EXPECT_EQ(edge.label, 5);
}

TEST_F(ExtendedEdgeTest, Equality) {
    gspan::ExtendedEdge edge1(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 5);
    gspan::ExtendedEdge edge2(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 5);
    gspan::ExtendedEdge edge3(gspan::Vertex{0, 1}, gspan::Vertex{1, 3}, 5);

    EXPECT_EQ(edge1, edge2);
    EXPECT_NE(edge1, edge3);
}

TEST_F(ExtendedEdgeTest, SmallerThan) {
    gspan::ExtendedEdge edge1(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1);
    gspan::ExtendedEdge edge2(gspan::Vertex{0, 1}, gspan::Vertex{1, 3}, 1);

    EXPECT_TRUE(edge1.SmallerThan(edge2));
    EXPECT_FALSE(edge2.SmallerThan(edge1));
}

class GraphParserTest : public ::testing::Test {};

TEST_F(GraphParserTest, ParseSingleGraph) {
    std::stringstream ss;
    ss << "t # 0\n";
    ss << "v 0 1\n";
    ss << "v 1 2\n";
    ss << "e 0 1 1\n";

    auto graphs = gspan::parser::ReadGraphs(ss);
    ASSERT_EQ(graphs.size(), 1);
    EXPECT_EQ(boost::num_vertices(graphs[0]), 2);
    EXPECT_EQ(boost::num_edges(graphs[0]), 1);
    bool found_v0 = false, found_v1 = false;
    for (auto v : boost::make_iterator_range(boost::vertices(graphs[0]))) {
        if (graphs[0][v].label == 1) found_v0 = true;
        if (graphs[0][v].label == 2) found_v1 = true;
    }
    EXPECT_TRUE(found_v0);
    EXPECT_TRUE(found_v1);
}

TEST_F(GraphParserTest, ParseEmptyGraph) {
    std::stringstream ss;
    ss << "t # 0\n";
    auto graphs = gspan::parser::ReadGraphs(ss);
    ASSERT_EQ(graphs.size(), 1);
    EXPECT_EQ(boost::num_vertices(graphs[0]), 0);
    EXPECT_EQ(boost::num_edges(graphs[0]), 0);
}

TEST_F(GraphParserTest, ParseSingleVertexGraph) {
    std::stringstream ss;
    ss << "t # 0\n";
    ss << "v 0 42\n";
    auto graphs = gspan::parser::ReadGraphs(ss);
    ASSERT_EQ(graphs.size(), 1);
    EXPECT_EQ(boost::num_vertices(graphs[0]), 1);
    EXPECT_EQ(boost::num_edges(graphs[0]), 0);
    bool found = false;
    for (auto v : boost::make_iterator_range(boost::vertices(graphs[0]))) {
        if (graphs[0][v].label == 42) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(GraphParserTest, ParseMultipleGraphs) {
    std::stringstream ss;
    ss << "t # 0\n";
    ss << "v 0 1\n";
    ss << "v 1 2\n";
    ss << "e 0 1 1\n";
    ss << "t # 1\n";
    ss << "v 0 3\n";
    ss << "v 1 4\n";
    ss << "v 2 5\n";
    ss << "e 0 1 2\n";
    ss << "e 1 2 3\n";

    auto graphs = gspan::parser::ReadGraphs(ss);
    ASSERT_EQ(graphs.size(), 2);
    EXPECT_EQ(boost::num_vertices(graphs[0]), 2);
    EXPECT_EQ(boost::num_edges(graphs[0]), 1);
    EXPECT_EQ(boost::num_vertices(graphs[1]), 3);
    EXPECT_EQ(boost::num_edges(graphs[1]), 2);
}

TEST_F(GraphParserTest, VertexLabels) {
    std::stringstream ss;
    ss << "t # 0\n";
    ss << "v 0 10\n";
    ss << "v 1 20\n";
    ss << "e 0 1 1\n";

    auto graphs = gspan::parser::ReadGraphs(ss);
    ASSERT_EQ(graphs.size(), 1);

    bool found_label_10 = false;
    bool found_label_20 = false;
    for (auto v : boost::make_iterator_range(boost::vertices(graphs[0]))) {
        if (graphs[0][v].label == 10) found_label_10 = true;
        if (graphs[0][v].label == 20) found_label_20 = true;
    }
    EXPECT_TRUE(found_label_10);
    EXPECT_TRUE(found_label_20);
}

class FrequentSubgraphTest : public ::testing::Test {};

TEST_F(FrequentSubgraphTest, Construction) {
    gspan::DFSCode code;
    code.Add(gspan::ExtendedEdge(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1));

    std::unordered_set<int> graph_ids{0, 1, 2};
    gspan::FrequentSubgraph subgraph(0, code, graph_ids, 3);

    EXPECT_EQ(subgraph.support, 3);
    EXPECT_EQ(subgraph.graphs_ids.size(), 3);
}

TEST_F(FrequentSubgraphTest, CompareTo) {
    gspan::DFSCode code;
    code.Add(gspan::ExtendedEdge(gspan::Vertex{0, 1}, gspan::Vertex{1, 2}, 1));

    gspan::FrequentSubgraph subgraph1(1, code, {0, 1, 2}, 3);
    gspan::FrequentSubgraph subgraph2(2, code, {0, 1}, 2);
    gspan::FrequentSubgraph subgraph3(3, code, {0, 1, 2}, 3);

    EXPECT_GT(subgraph1.CompareTo(subgraph2), 0);
    EXPECT_LT(subgraph2.CompareTo(subgraph1), 0);
    EXPECT_EQ(subgraph1.CompareTo(subgraph3), 0);
}

class GSpanTest : public ::testing::Test {
protected:
    template <typename... Args>
    static std::unique_ptr<algos::GSpan> CreateAlgorithmInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::GSpan>(
                CreateGSpanParams(std::forward<Args>(args)...));
    }
};

TEST_F(GSpanTest, LargeGraph) {
    auto algorithm = CreateAlgorithmInstance(kGSpanLargeGraph, 0.5);
    algorithm->Execute();

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();
    EXPECT_GE(subgraphs.size(), 1);
    for (auto const& sg : subgraphs) {
        EXPECT_GE(sg.support, algorithm->GetMinSup());
    }
}

TEST_F(GSpanTest, SimpleDatasetBasicExecution) {
    auto algorithm = CreateAlgorithmInstance(kGSpanTestSimple, 0.6);
    algorithm->Execute();

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();
    EXPECT_GE(subgraphs.size(), 1);

    for (auto const& sg : subgraphs) {
        EXPECT_GE(sg.support, algorithm->GetMinSup());
    }
}

TEST_F(GSpanTest, HighMinimumSupport) {
    auto algorithm = CreateAlgorithmInstance(kGSpanTestSimple, 1.0);
    algorithm->Execute();

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();

    for (auto const& sg : subgraphs) {
        EXPECT_EQ(sg.support, 5);
    }
}

TEST_F(GSpanTest, LowMinimumSupport) {
    auto algorithm_low = CreateAlgorithmInstance(kGSpanTestSimple, 0.2);
    algorithm_low->Execute();

    auto algorithm_high = CreateAlgorithmInstance(kGSpanTestSimple, 0.8);
    algorithm_high->Execute();

    EXPECT_GE(algorithm_low->GetFrequentSubgraphs().size(),
              algorithm_high->GetFrequentSubgraphs().size());
}

TEST_F(GSpanTest, TrianglePatternMining) {
    auto algorithm = CreateAlgorithmInstance(kGSpanTestTriangle, 0.6);
    algorithm->Execute();

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();

    EXPECT_GE(subgraphs.size(), 1);

    bool found_high_support = false;
    for (auto const& sg : subgraphs) {
        if (sg.support >= 4) {
            found_high_support = true;
            break;
        }
    }
    EXPECT_TRUE(found_high_support);
}

TEST_F(GSpanTest, ChainPatternMining) {
    auto algorithm = CreateAlgorithmInstance(kGSpanTestChain, 0.6);
    algorithm->Execute();

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();

    EXPECT_GE(subgraphs.size(), 1);

    for (auto const& sg : subgraphs) {
        EXPECT_GE(sg.support, 3);
    }
}

TEST_F(GSpanTest, MaxEdgesConstraint) {
    auto algorithm_small = CreateAlgorithmInstance(kGSpanTestSimple, 0.4, false, 1);
    algorithm_small->Execute();

    auto algorithm_large = CreateAlgorithmInstance(kGSpanTestSimple, 0.4, false, 10);
    algorithm_large->Execute();

    auto const& small_subgraphs = algorithm_small->GetFrequentSubgraphs();
    auto const& large_subgraphs = algorithm_large->GetFrequentSubgraphs();

    for (auto const& sg : small_subgraphs) {
        EXPECT_LE(sg.dfs_code.Size(), 1);
    }

    EXPECT_GE(large_subgraphs.size(), small_subgraphs.size());
}

struct GSpanTestParams {
    std::filesystem::path graph_path;
    double min_support;
    bool should_find_patterns;
    std::string description;

    GSpanTestParams(std::filesystem::path path, double support, bool find_patterns,
                    std::string desc)
        : graph_path(std::move(path)),
          min_support(support),
          should_find_patterns(find_patterns),
          description(std::move(desc)) {}
};

class GSpanParameterizedTest : public ::testing::TestWithParam<GSpanTestParams> {};

TEST_P(GSpanParameterizedTest, ExecuteWithParams) {
    auto const& params = GetParam();
    auto algorithm = algos::CreateAndLoadAlgorithm<algos::GSpan>(
            CreateGSpanParams(params.graph_path, params.min_support));

    ASSERT_NO_THROW(algorithm->Execute());

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();
    int min_sup = algorithm->GetMinSup();

    for (auto const& sg : subgraphs) {
        EXPECT_GE(sg.support, min_sup)
                << "Subgraph has support " << sg.support << " but min_sup is " << min_sup;
    }

    if (params.should_find_patterns && min_sup > 0) {
        if (params.min_support < 1.0) {
            EXPECT_GT(subgraphs.size(), 0) << "Expected to find frequent subgraphs";
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
        GSpanTestSuite, GSpanParameterizedTest,
        ::testing::Values(GSpanTestParams(kGSpanTestSimple, 0.2, true, "Simple_LowSupport"),
                          GSpanTestParams(kGSpanTestSimple, 0.4, true, "Simple_MediumSupport"),
                          GSpanTestParams(kGSpanTestSimple, 0.6, true, "Simple_HighSupport"),
                          GSpanTestParams(kGSpanTestSimple, 0.8, true, "Simple_VeryHighSupport"),
                          GSpanTestParams(kGSpanTestTriangle, 0.4, true, "Triangle_MediumSupport"),
                          GSpanTestParams(kGSpanTestTriangle, 0.6, true, "Triangle_HighSupport"),
                          GSpanTestParams(kGSpanTestChain, 0.4, true, "Chain_MediumSupport"),
                          GSpanTestParams(kGSpanTestChain, 0.8, true, "Chain_HighSupport")),
        [](testing::TestParamInfo<GSpanTestParams> const& info) { return info.param.description; });

class GSpanEdgeCasesTest : public ::testing::Test {};

TEST_F(GSpanEdgeCasesTest, MinimumSupportBoundary) {
    auto algorithm =
            algos::CreateAndLoadAlgorithm<algos::GSpan>(CreateGSpanParams(kGSpanTestSimple, 0.01));
    EXPECT_NO_THROW(algorithm->Execute());
}

TEST_F(GSpanEdgeCasesTest, InvalidMinimumSupportZero) {
    EXPECT_THROW(
            algos::CreateAndLoadAlgorithm<algos::GSpan>(CreateGSpanParams(kGSpanTestSimple, 0.0)),
            config::ConfigurationError);
}

TEST_F(GSpanEdgeCasesTest, InvalidMinimumSupportNegative) {
    EXPECT_THROW(
            algos::CreateAndLoadAlgorithm<algos::GSpan>(CreateGSpanParams(kGSpanTestSimple, -0.1)),
            config::ConfigurationError);
}

TEST_F(GSpanEdgeCasesTest, InvalidMinimumSupportGreaterThanOne) {
    EXPECT_THROW(
            algos::CreateAndLoadAlgorithm<algos::GSpan>(CreateGSpanParams(kGSpanTestSimple, 1.5)),
            config::ConfigurationError);
}

TEST_F(GSpanEdgeCasesTest, InvalidMaxEdgesZero) {
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<algos::GSpan>(
                         CreateGSpanParams(kGSpanTestSimple, 0.5, false, 0)),
                 config::ConfigurationError);
}

TEST_F(GSpanEdgeCasesTest, InvalidMaxEdgesNegative) {
    EXPECT_THROW(algos::CreateAndLoadAlgorithm<algos::GSpan>(
                         CreateGSpanParams(kGSpanTestSimple, 0.5, false, -1)),
                 config::ConfigurationError);
}

class GSpanCorrectnessTest : public ::testing::Test {};

TEST_F(GSpanCorrectnessTest, AllSubgraphsMeetMinSupport) {
    std::vector<double> supports = {0.2, 0.4, 0.6, 0.8};

    for (double support : supports) {
        auto algorithm = algos::CreateAndLoadAlgorithm<algos::GSpan>(
                CreateGSpanParams(kGSpanTestSimple, support));
        algorithm->Execute();

        int min_sup = algorithm->GetMinSup();
        auto const& subgraphs = algorithm->GetFrequentSubgraphs();

        for (auto const& sg : subgraphs) {
            EXPECT_GE(sg.support, min_sup)
                    << "With support=" << support << ", found subgraph with support " << sg.support
                    << " but min_sup is " << min_sup;

            EXPECT_EQ(static_cast<int>(sg.graphs_ids.size()), sg.support)
                    << "graph_ids.size() should equal support";
        }
    }
}

TEST_F(GSpanCorrectnessTest, CanonicalFormConsistency) {
    auto algorithm =
            algos::CreateAndLoadAlgorithm<algos::GSpan>(CreateGSpanParams(kGSpanTestTriangle, 0.4));
    ASSERT_NO_THROW(algorithm->Execute());

    auto const& subgraphs = algorithm->GetFrequentSubgraphs();

    for (size_t i = 0; i < subgraphs.size(); ++i) {
        for (size_t j = i + 1; j < subgraphs.size(); ++j) {
            auto const& code_i = subgraphs[i].dfs_code;
            auto const& code_j = subgraphs[j].dfs_code;

            if (code_i.Size() == code_j.Size()) {
                bool all_same = true;
                for (size_t k = 0; k < code_i.Size(); ++k) {
                    if (!(code_i[k] == code_j[k])) {
                        all_same = false;
                        break;
                    }
                }
                EXPECT_FALSE(all_same)
                        << "Found duplicate patterns at indices " << i << " and " << j;
            }
        }
    }
}

}  // namespace tests
