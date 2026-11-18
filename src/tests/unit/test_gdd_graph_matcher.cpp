#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/range/iterator_range.hpp>
#include <gtest/gtest.h>

#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/algorithms/gdd/graph_matcher/backtracking_graph_matcher.h"
#include "core/algorithms/gdd/graph_matcher/graph_matcher.h"
#include "core/algorithms/gdd/graph_matcher/wcoj_graph_matcher.h"
#include "test_gdd_utils.h"

namespace tests {

using Graph = model::gdd::GraphTypes::Graph;
using Vertex = model::gdd::GraphTypes::Vertex;
using Mapping = model::gdd::GraphTypes::Mapping;

using gdd::utils::AddEdge;
using gdd::utils::AddVertex;
using gdd::utils::FindVertexById;
using gdd::utils::MakeArrowPattern;
using gdd::utils::ReadGraphFromDot;

enum class Kind : std::uint8_t { kBacktracking, kWcoj };

std::string KindName(Kind kind) {
    return kind == Kind::kBacktracking ? "Backtracking" : "Wcoj";
}

std::ostream& operator<<(std::ostream& os, Kind kind) {
    return os << KindName(kind);
}

using Tuple = std::vector<std::size_t>;
using Matches = std::multiset<Tuple>;  // repeated matches must not be ignored

using Callback = std::function<bool(Mapping const&)>;
using Matcher = algos::GraphMatcher<Callback>;
using algos::MatchSemantics;

std::unique_ptr<Matcher> MakeMatcher(Kind kind, Graph const& g, Callback cb) {
    if (kind == Kind::kBacktracking) {
        return std::make_unique<algos::BacktrackingGraphMatcher<Callback>>(g, std::move(cb));
    }
    return std::make_unique<algos::WcojGraphMatcher<Callback>>(g, std::move(cb));
}

// apply mapping and retrieve id's in pattern vertex id's order
// `Mapping` is dependent on order of vertex addition to graph, vertex id is not
Tuple ToTuple(Graph const& p, Graph const& g, Mapping const& f) {
    EXPECT_EQ(f.size(), boost::num_vertices(p));
    std::vector pv(boost::vertices(p).first, boost::vertices(p).second);
    std::ranges::sort(pv, {}, [&p](Vertex v) { return p[v].id; });
    Tuple t;
    for (Vertex v : pv) {
        t.push_back(g[f.at(v)].id);
    }
    return t;
}

Matches CollectMatches(Kind kind, Graph const& g, Graph const& p, MatchSemantics semantics) {
    Matches out;
    auto const matcher = MakeMatcher(kind, g, [&](Mapping const& f) {
        out.insert(ToTuple(p, g, f));
        return true;
    });
    EXPECT_TRUE(matcher->Run(p, semantics));
    return out;
}

struct Case {
    std::string name;
    std::string pattern_dot;
    std::string graph_dot;
    Matches homomorphisms;
    Matches isomorphisms;
};

std::ostream& operator<<(std::ostream& os, Case const& tc) {
    return os << tc.name;
}

std::string const kOutStarPattern = R"(digraph P {
    1 [label="A"]; 2 [label="B"]; 3 [label="B"];
    1 -> 2 [label="l"]; 1 -> 3 [label="l"];
})";

std::string const kOutStarGraph = kOutStarPattern;

Matches const kOutStarHomomorphisms = {{1, 2, 2}, {1, 2, 3}, {1, 3, 2}, {1, 3, 3}};
Matches const kOutStarIsomorphisms = {{1, 2, 3}, {1, 3, 2}};

std::string const kMergePattern = R"(digraph P {
    1 [label="X"]; 2 [label="X"];
    1 -> 2 [label="l"];
})";

std::string const kMergeGraph = R"(digraph G {
    1 [label="X"]; 2 [label="X"];
    1 -> 1 [label="l"]; 1 -> 2 [label="l"];
})";

std::vector<Case> GetCases() {
    return {
            {.name = "EmptyPattern",
             .pattern_dot = "digraph P {}",
             .graph_dot = R"(digraph G { 1 [label="A"]; })",
             .homomorphisms = {Tuple{}},
             .isomorphisms = {Tuple{}}},
            {.name = "SingleVertex",
             .pattern_dot = R"(digraph P { 1 [label="A"]; })",
             .graph_dot = R"(digraph G { 1 [label="A"]; 2 [label="B"]; 3 [label="A"]; })",
             .homomorphisms = {{1}, {3}},
             .isomorphisms = {{1}, {3}}},
            {.name = "IsolatedDifferentLabels",
             .pattern_dot = R"(digraph P { 1 [label="A"]; 2 [label="B"]; })",
             .graph_dot = R"(digraph G { 1 [label="A"]; 2 [label="B"]; 3 [label="A"]; })",
             .homomorphisms = {{1, 2}, {3, 2}},
             .isomorphisms = {{1, 2}, {3, 2}}},
            {.name = "IsolatedEmptyFactor",
             .pattern_dot = R"(digraph P { 1 [label="A"]; 2 [label="C"]; })",
             .graph_dot = R"(digraph G { 1 [label="A"]; 2 [label="B"]; })",
             .homomorphisms = {},
             .isomorphisms = {}},
            {.name = "EdgeCondition",
             .pattern_dot = R"(digraph P {
                 1 [label="A"]; 2 [label="B"];
                 1 -> 2 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="A"]; 2 [label="B"]; 3 [label="B"]; 4 [label="B"]; 5 [label="B"];
                 1 -> 2 [label="l"]; 1 -> 2 [label="l"]; 1 -> 2 [label="m"];
                 1 -> 3 [label="m"];
                 4 -> 1 [label="l"];
             })",
             .homomorphisms = {{1, 2}},
             .isomorphisms = {{1, 2}}},
            {.name = "EdgeLabelAbsent",
             .pattern_dot = R"(digraph P {
                 1 [label="A"]; 2 [label="B"];
                 1 -> 2 [label="m"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="A"]; 2 [label="B"];
                 1 -> 2 [label="l"];
             })",
             .homomorphisms = {},
             .isomorphisms = {}},
            {.name = "EndpointWithoutCandidates",
             .pattern_dot = R"(digraph P {
                 1 [label="A"]; 2 [label="C"];
                 1 -> 2 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="A"]; 2 [label="B"];
                 1 -> 2 [label="l"];
             })",
             .homomorphisms = {},
             .isomorphisms = {}},
            {.name = "EdgeMultipleConditions",
             .pattern_dot = R"(digraph P {
                 1 [label="A"]; 2 [label="B"];
                 1 -> 2 [label="l"]; 1 -> 2 [label="l"]; 1 -> 2 [label="m"];
                 2 -> 1 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="A"]; 2 [label="B"]; 3 [label="B"]; 4 [label="B"]; 5 [label="B"];
                 1 -> 2 [label="l"]; 1 -> 2 [label="m"]; 2 -> 1 [label="l"];
                 1 -> 3 [label="l"]; 1 -> 3 [label="m"];
                 1 -> 4 [label="l"]; 4 -> 1 [label="l"];
                 1 -> 5 [label="m"]; 5 -> 1 [label="l"];
             })",
             .homomorphisms = {{1, 2}},
             .isomorphisms = {{1, 2}}},
            {.name = "MergeRequiresLoop",
             .pattern_dot = kMergePattern,
             .graph_dot = kMergeGraph,
             .homomorphisms = {{1, 1}, {1, 2}},
             .isomorphisms = {{1, 2}}},
            {.name = "PatternLoops",
             .pattern_dot = R"(digraph P {
                 1 [label="X"]; 2 [label="Y"];
                 1 -> 1 [label="l"]; 1 -> 2 [label="m"]; 2 -> 2 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="X"]; 2 [label="X"]; 3 [label="X"]; 4 [label="Y"]; 5 [label="Y"];
                 1 -> 1 [label="l"]; 2 -> 2 [label="m"]; 4 -> 4 [label="l"];
                 3 -> 1 [label="l"];
                 1 -> 4 [label="m"]; 1 -> 5 [label="m"]; 2 -> 4 [label="m"]; 3 -> 4 [label="m"];
             })",
             .homomorphisms = {{1, 4}},
             .isomorphisms = {{1, 4}}},
            {.name = "OutStar",
             .pattern_dot = kOutStarPattern,
             .graph_dot = kOutStarGraph,
             .homomorphisms = kOutStarHomomorphisms,
             .isomorphisms = kOutStarIsomorphisms},
            {.name = "NeighbourListIntersection",
             .pattern_dot = R"(digraph P {
                 1 [label="A"]; 2 [label="B"]; 3 [label="D"]; 4 [label="C"];
                 1 -> 4 [label="l"]; 4 -> 2 [label="l"]; 3 -> 4 [label="m"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="A"]; 2 [label="B"]; 3 [label="D"];
                 11 [label="C"]; 12 [label="C"]; 13 [label="C"];
                 14 [label="C"]; 15 [label="C"]; 16 [label="E"];
                 1 -> 11 [label="l"]; 11 -> 2 [label="l"]; 3 -> 11 [label="m"];
                                      12 -> 2 [label="l"]; 3 -> 12 [label="m"];
                 1 -> 13 [label="l"];                      3 -> 13 [label="m"];
                 1 -> 14 [label="l"]; 14 -> 2 [label="l"]; 3 -> 14 [label="l"];
                 1 -> 15 [label="l"]; 15 -> 2 [label="l"]; 3 -> 15 [label="m"];
                 1 -> 16 [label="l"]; 16 -> 2 [label="l"]; 3 -> 16 [label="m"];
             })",
             .homomorphisms = {{1, 2, 3, 11}, {1, 2, 3, 15}},
             .isomorphisms = {{1, 2, 3, 11}, {1, 2, 3, 15}}},
            {.name = "Triangle",
             .pattern_dot = R"(digraph P {
                 1 [label="X"]; 2 [label="X"]; 3 [label="X"];
                 1 -> 2 [label="l"]; 2 -> 3 [label="l"]; 3 -> 1 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="X"]; 2 [label="X"]; 3 [label="X"];
                 4 [label="X"]; 5 [label="X"]; 6 [label="X"];
                 1 -> 2 [label="l"]; 2 -> 3 [label="l"]; 3 -> 1 [label="l"];
                 4 -> 5 [label="l"]; 5 -> 6 [label="l"]; 4 -> 6 [label="l"];
             })",
             .homomorphisms = {{1, 2, 3}, {2, 3, 1}, {3, 1, 2}},
             .isomorphisms = {{1, 2, 3}, {2, 3, 1}, {3, 1, 2}}},
            {.name = "Components",
             .pattern_dot = R"(digraph P {
                 1 [label="A"]; 2 [label="A"]; 3 [label="A"]; 4 [label="A"];
                 1 -> 2 [label="l"]; 2 -> 1 [label="l"]; 3 -> 4 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="A"]; 2 [label="A"]; 3 [label="A"]; 4 [label="A"];
                 1 -> 2 [label="l"]; 2 -> 1 [label="l"]; 3 -> 4 [label="l"];
             })",
             .homomorphisms = {{1, 2, 1, 2},
                               {1, 2, 2, 1},
                               {1, 2, 3, 4},
                               {2, 1, 1, 2},
                               {2, 1, 2, 1},
                               {2, 1, 3, 4}},
             .isomorphisms = {{1, 2, 3, 4}, {2, 1, 3, 4}}},
            {.name = "LongPath",
             .pattern_dot = R"(digraph P {
                 1 [label="X"]; 2 [label="X"]; 3 [label="X"]; 4 [label="X"];
                 5 [label="X"]; 6 [label="X"]; 7 [label="X"]; 8 [label="Y"];
                 1 -> 2 [label="l"]; 2 -> 3 [label="l"]; 3 -> 4 [label="l"]; 4 -> 5 [label="l"];
                 5 -> 6 [label="l"]; 6 -> 7 [label="l"]; 7 -> 8 [label="l"];
             })",
             .graph_dot = R"(digraph G {
                 1 [label="X"]; 2 [label="X"]; 3 [label="X"]; 10 [label="Y"];
                 1 -> 2 [label="l"]; 2 -> 3 [label="l"]; 3 -> 1 [label="l"];
                 3 -> 10 [label="l"];
             })",
             .homomorphisms = {{3, 1, 2, 3, 1, 2, 3, 10}},
             .isomorphisms = {}},
    };
}

class GraphMatcherCases : public ::testing::TestWithParam<std::tuple<Kind, Case>> {};

TEST_P(GraphMatcherCases, MatchesDefinition) {
    auto const& [kind, tc] = GetParam();
    Graph const p = ReadGraphFromDot(tc.pattern_dot);
    Graph const g = ReadGraphFromDot(tc.graph_dot);
    EXPECT_EQ(CollectMatches(kind, g, p, MatchSemantics::kHomomorphism), tc.homomorphisms)
            << tc.name << ", " << kind << ", homomorphism";
    EXPECT_EQ(CollectMatches(kind, g, p, MatchSemantics::kIsomorphism), tc.isomorphisms)
            << tc.name << ", " << kind << ", isomorphism";
}

INSTANTIATE_TEST_SUITE_P(GraphMatcher, GraphMatcherCases,
                         ::testing::Combine(::testing::Values(Kind::kBacktracking, Kind::kWcoj),
                                            ::testing::ValuesIn(GetCases())),
                         [](auto const& info) {
                             return std::get<1>(info.param).name + "_" +
                                    KindName(std::get<0>(info.param));
                         });

class GraphMatcherContract : public ::testing::TestWithParam<Kind> {};

TEST_P(GraphMatcherContract, EarlyStop) {
    Graph const p = ReadGraphFromDot(kOutStarPattern);
    Graph const g = ReadGraphFromDot(kOutStarGraph);
    Matches out;
    std::size_t calls = 0;
    std::size_t stop_at = 1;
    auto matcher = MakeMatcher(GetParam(), g, [&](Mapping const& f) {
        out.insert(ToTuple(p, g, f));
        return ++calls != stop_at;
    });

    EXPECT_FALSE(matcher->Run(p));
    EXPECT_EQ(calls, 1);
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(kOutStarHomomorphisms.contains(*out.begin()));

    out.clear();
    calls = 0;
    stop_at = 4;
    EXPECT_FALSE(matcher->Run(p));
    EXPECT_EQ(calls, 4);
    EXPECT_EQ(out, kOutStarHomomorphisms);

    out.clear();
    calls = 0;
    stop_at = 0;
    EXPECT_TRUE(matcher->Run(p));
    EXPECT_EQ(out, kOutStarHomomorphisms);
}

TEST_P(GraphMatcherContract, Reuse) {
    Graph const g = ReadGraphFromDot(kOutStarGraph);
    Graph const p_star = ReadGraphFromDot(kOutStarPattern);
    Graph const p_m = MakeArrowPattern(1, "A", 2, "B", "m");
    Graph const p_l = MakeArrowPattern(1, "A", 2, "B", "l");
    Graph const* cur = nullptr;
    Matches out;
    auto matcher = MakeMatcher(GetParam(), g, [&](Mapping const& f) {
        out.insert(ToTuple(*cur, g, f));
        return true;
    });
    auto run = [&](Graph const& p, MatchSemantics semantics) {
        cur = &p;
        out.clear();
        EXPECT_TRUE(matcher->Run(p, semantics));
        return out;
    };

    EXPECT_EQ(run(p_star, MatchSemantics::kHomomorphism), kOutStarHomomorphisms);
    EXPECT_EQ(run(p_star, MatchSemantics::kIsomorphism), kOutStarIsomorphisms);
    EXPECT_EQ(run(p_m, MatchSemantics::kIsomorphism), Matches{});
    EXPECT_EQ(run(p_l, MatchSemantics::kHomomorphism), (Matches{{1, 2}, {1, 3}}));
}

TEST_P(GraphMatcherContract, Reset) {
    Graph g = MakeArrowPattern(1, "A", 2, "B", "l");
    Graph const p = MakeArrowPattern(0, "A", 1, "B", "l");
    Matches out;
    auto const matcher = MakeMatcher(GetParam(), g, [&](Mapping const& f) {
        out.insert(ToTuple(p, g, f));
        return true;
    });

    matcher->Reset();
    EXPECT_TRUE(matcher->Run(p));
    EXPECT_EQ(out, (Matches{{1, 2}}));

    Vertex const v3 = AddVertex(g, 3, "B");
    AddEdge(g, FindVertexById(g, 1), v3, "l");
    matcher->Reset();
    out.clear();
    EXPECT_TRUE(matcher->Run(p));
    EXPECT_EQ(out, (Matches{{1, 2}, {1, 3}}));

    matcher->Reset();
    matcher->Reset();
    out.clear();
    EXPECT_TRUE(matcher->Run(p));
    EXPECT_EQ(out, (Matches{{1, 2}, {1, 3}}));
}

struct GraphMatcherTestError : std::runtime_error {
    GraphMatcherTestError() : std::runtime_error("GraphMatcher test error") {}
};

TEST_P(GraphMatcherContract, CallbackThrows) {
    Graph const p = ReadGraphFromDot(kOutStarPattern);
    Graph const g = ReadGraphFromDot(kOutStarGraph);
    Matches out;
    bool should_throw = true;
    auto const matcher = MakeMatcher(GetParam(), g, [&](Mapping const& f) {
        if (should_throw) {
            throw GraphMatcherTestError{};
        }
        out.insert(ToTuple(p, g, f));
        return true;
    });

    EXPECT_THROW(matcher->Run(p), GraphMatcherTestError);
    should_throw = false;
    EXPECT_TRUE(matcher->Run(p));
    EXPECT_EQ(out, kOutStarHomomorphisms);
}

TEST_P(GraphMatcherContract, PatternLifetime) {
    auto const label = std::string(100, 'x');  // longer than sso buffer
    Graph const g = MakeArrowPattern(1, "A", 2, "B", label);
    Graph const* cur = nullptr;
    Matches out;
    auto const matcher = MakeMatcher(GetParam(), g, [&](Mapping const& f) {
        out.insert(ToTuple(*cur, g, f));
        return true;
    });

    for (std::size_t i = 0; i < 100; ++i) {
        auto const p = std::make_unique<Graph>(MakeArrowPattern(i, "A", i + 1, "B", label));
        cur = p.get();
        out.clear();
        EXPECT_TRUE(matcher->Run(*p));
        EXPECT_EQ(out, (Matches{{1, 2}}));
    }
}

INSTANTIATE_TEST_SUITE_P(GraphMatcher, GraphMatcherContract,
                         ::testing::Values(Kind::kBacktracking, Kind::kWcoj),
                         [](auto const& info) { return KindName(info.param); });

}  // namespace tests
