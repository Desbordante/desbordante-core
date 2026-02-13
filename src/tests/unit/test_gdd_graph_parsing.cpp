#include <sstream>
#include <stdexcept>

#include <boost/range/iterator_range.hpp>
#include <gtest/gtest.h>

#include "core/parser/graph_parser/graph_parser.h"

namespace tests {

namespace {

model::gdd::vertex_t FindVertexById(model::gdd::graph_t const& g, int id) {
    for (auto const v : boost::make_iterator_range(boost::vertices(g))) {
        if (g[v].id == static_cast<size_t>(id)) return v;
    }
    throw std::runtime_error("vertex with requested id not found");
}

}  // namespace

TEST(GddGraphParser, ParsesNumericVertexIdsLabelsAttributesAndEdgeLabels) {
    std::stringstream ss;
    ss << R"(digraph G {
               10 [label="A", foo="bar"];
               2  [label="B"];
               10 -> 2 [label="e"];
             })";

    model::gdd::graph_t const g = parser::graph_parser::gdd::ReadGraph(ss);

    ASSERT_EQ(boost::num_vertices(g), 2u);
    ASSERT_EQ(boost::num_edges(g), 1u);

    auto const v10 = FindVertexById(g, 10);
    auto const v2 = FindVertexById(g, 2);

    EXPECT_EQ(g[v10].label, "A");
    EXPECT_EQ(g[v2].label, "B");

    EXPECT_TRUE(!g[v10].attributes.contains("label"));
    ASSERT_TRUE(g[v10].attributes.contains("foo"));
    EXPECT_EQ(g[v10].attributes.at("foo"), "bar");

    EXPECT_TRUE(g[v2].attributes.empty());

    auto const [e, ok] = boost::edge(v10, v2, g);
    ASSERT_TRUE(ok);
    EXPECT_EQ(g[e].label, "e");
}

TEST(GddGraphParser, VertexWithoutLabelGivesEmptyLabelAndKeepsOtherAttributes) {
    std::stringstream ss;
    ss << R"(digraph G {
               0 [name="x"];
             })";

    model::gdd::graph_t const g = parser::graph_parser::gdd::ReadGraph(ss);

    ASSERT_EQ(boost::num_vertices(g), 1u);

    auto const v0 = FindVertexById(g, 0);
    EXPECT_EQ(g[v0].label, "");

    ASSERT_TRUE(g[v0].attributes.contains("name"));
    EXPECT_EQ(g[v0].attributes.at("name"), "x");
}

TEST(GddGraphParser, NonIntegerVertexIdThrows) {
    std::stringstream ss;
    ss << R"(digraph G {
               n0 [label="A"];
             })";

    EXPECT_THROW((parser::graph_parser::gdd::ReadGraph(ss)), std::runtime_error);
}

TEST(GddGraphParser, SameNumericIdDoesNotCreateDuplicateVertices) {
    std::stringstream ss;
    ss << R"(digraph G {
               1 [label="A", x="1"];
               1 [label="A2", y="2"];
             })";

    model::gdd::graph_t const g = parser::graph_parser::gdd::ReadGraph(ss);
    ASSERT_EQ(boost::num_vertices(g), 1u);

    auto const v1 = FindVertexById(g, 1);
    EXPECT_EQ(g[v1].id, 1);
}

}  // namespace tests
