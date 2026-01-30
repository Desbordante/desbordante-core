#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/graph/graph_traits.hpp>
#include <gtest/gtest.h>

#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"

namespace tests {

namespace {

using model::Gdd;
using model::gdd::graph_t;
using model::gdd::vertex_t;

using model::gdd::detail::AttrTag;
using model::gdd::detail::CmpOp;
using model::gdd::detail::ConstValue;
using model::gdd::detail::DistanceConstraint;
using model::gdd::detail::DistanceMetric;
using model::gdd::detail::DistanceOperand;
using model::gdd::detail::GddToken;
using model::gdd::detail::RelTag;

vertex_t AddVertex(graph_t& g, std::uint64_t id, std::string label,
                   std::unordered_map<std::string, std::string> attrs = {}) {
    auto const v = boost::add_vertex(g);
    g[v].id = id;
    g[v].label = std::move(label);
    g[v].attributes = std::move(attrs);
    return v;
}

void AddEdge(graph_t& g, vertex_t from, vertex_t to, std::string label) {
    auto [e, ok] = boost::add_edge(from, to, g);
    ASSERT_TRUE(ok);
    g[e].label = std::move(label);
}

graph_t MakeSingleVertexPattern(size_t pid = 1, std::string label = "X",
                                std::unordered_map<std::string, std::string> attrs = {}) {
    graph_t p;
    auto const v = boost::add_vertex(p);
    p[v].id = pid;
    p[v].label = std::move(label);
    p[v].attributes = std::move(attrs);
    return p;
}

graph_t MakeTwoVertexPattern(size_t pid1 = 1, size_t pid2 = 2, std::string l1 = "A",
                             std::string l2 = "B") {
    graph_t p;
    auto const v1 = boost::add_vertex(p);
    auto const v2 = boost::add_vertex(p);
    p[v1].id = pid1;
    p[v1].label = std::move(l1);
    p[v2].id = pid2;
    p[v2].label = std::move(l2);
    return p;
}

vertex_t FindPatternVertex(graph_t const& p, size_t pid) {
    for (auto [it, end] = boost::vertices(p); it != end; ++it) {
        if (p[*it].id == pid) return *it;
    }
    throw std::logic_error("pattern vertex not found");
}

DistanceConstraint AttrConst(size_t pid, std::string attr, ConstValue c, DistanceMetric metric,
                             CmpOp op, double t) {
    return DistanceConstraint{
            .lhs = GddToken{pid, AttrTag{std::move(attr)}},
            .rhs = std::move(c),
            .threshold = t,
            .metric = metric,
            .op = op,
    };
}

DistanceConstraint AttrAttr(size_t pid1, std::string a1, size_t pid2, std::string a2,
                            DistanceMetric metric, CmpOp op, double t) {
    return DistanceConstraint{
            .lhs = GddToken{pid1, AttrTag{std::move(a1)}},
            .rhs = GddToken{pid2, AttrTag{std::move(a2)}},
            .threshold = t,
            .metric = metric,
            .op = op,
    };
}

DistanceConstraint RelConst(size_t pid, std::string rela, ConstValue cr) {
    return DistanceConstraint{
            .lhs = GddToken{pid, RelTag{std::move(rela)}},
            .rhs = std::move(cr),
            .threshold = 0.0,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kEq,
    };
}

DistanceConstraint RelRel(size_t pid1, std::string rela1, size_t pid2, std::string rela2) {
    return DistanceConstraint{
            .lhs = GddToken{pid1, RelTag{std::move(rela1)}},
            .rhs = GddToken{pid2, RelTag{std::move(rela2)}},
            .threshold = 0.0,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kEq,
    };
}

}  // namespace

TEST(GddSatisfiesConstraint, EmptyLhsRhsSatisfies) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X");
    auto const pv = FindPatternVertex(p, 1);

    std::unordered_map<vertex_t, vertex_t> map;
    map.emplace(pv, gv);

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, LhsFalseThenSatisfies) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const l =
            AttrConst(1, "id", 10LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);
    DistanceConstraint const r = RelRel(230498, "randomstr", 1234, "anotherrandomstr");

    Gdd const gdd(p, Gdd::Phi{l}, Gdd::Phi{r, r, r, r});

    graph_t g;
    AddVertex(g, 10, "X");

    EXPECT_TRUE(gdd.Satisfies(g, {}));
}

TEST(GddSatisfiesConstraint, MissingPatternVertexInMapMakesConstraintFail) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "id", 10LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);

    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    AddVertex(g, 10, "X");

    EXPECT_FALSE(gdd.Satisfies(g, {}));
}

TEST(GddSatisfiesConstraint, AttrConst_AbsDiff_UsesNumericParsing) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "id", 10LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X");
    auto const pv = FindPatternVertex(p, 1);

    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};
    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrConst_AbsDiff_FailsWhenBeyondThreshold) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "id", 10LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 11, "X");
    auto const pv = FindPatternVertex(p, 1);

    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};
    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrConst_AbsDiff_AllowsDistanceWithinThreshold) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "id", 10LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 1.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 11, "X");
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrConst_EditDistance_Strings) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "label", std::string("kitten"),
                                           DistanceMetric::kEditDistance, CmpOp::kLe, 3.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 1, "sitting");
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrConst_EditDistance_FailsWhenTooLarge) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "label", std::string("kitten"),
                                           DistanceMetric::kEditDistance, CmpOp::kLe, 2.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 1, "sitting");
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrAttr_AbsDiff_BetweenTwoMappedVertices) {
    graph_t p = MakeTwoVertexPattern(1, 2, "A", "B");

    // |x1.id - x2.id| <= 2
    DistanceConstraint const c =
            AttrAttr(1, "id", 2, "id", DistanceMetric::kAbsDiff, CmpOp::kLe, 2.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const g1 = AddVertex(g, 10, "A");
    auto const g2 = AddVertex(g, 12, "B");
    auto const p1 = FindPatternVertex(p, 1);
    auto const p2 = FindPatternVertex(p, 2);

    std::unordered_map<vertex_t, vertex_t> const map{{p1, g1}, {p2, g2}};
    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrAttr_AbsDiff_FailsBetweenTwoMappedVertices) {
    graph_t p = MakeTwoVertexPattern(1, 2, "A", "B");

    DistanceConstraint const c =
            AttrAttr(1, "id", 2, "id", DistanceMetric::kAbsDiff, CmpOp::kLe, 1.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const g1 = AddVertex(g, 10, "A");
    auto const g2 = AddVertex(g, 12, "B");
    auto const p1 = FindPatternVertex(p, 1);
    auto const p2 = FindPatternVertex(p, 2);

    std::unordered_map<vertex_t, vertex_t> const map{{p1, g1}, {p2, g2}};
    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AttrMissingInGraphReturnsFalse) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "color", 0LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X");  // no "color" attribute
    auto const pv = FindPatternVertex(p, 1);

    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};
    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, AbsDiffWithStringConstThrows) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "label", std::string("abc"), DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 1, "abc");
    auto const pv = FindPatternVertex(p, 1);

    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_THROW(gdd.Satisfies(g, map), std::logic_error);
}

TEST(GddSatisfiesConstraint, RelConst_SatisfiedWhenEdgeEndsAtCr) {
    graph_t p = MakeSingleVertexPattern(1, "A");
    DistanceConstraint const c = RelConst(1, "knows", 42LL);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const b = AddVertex(g, 42, "B");
    AddEdge(g, a, b, "knows");

    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, a}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, RelConst_FailsWhenNoSuchEdge) {
    graph_t p = MakeSingleVertexPattern(1, "A");
    DistanceConstraint const c = RelConst(1, "knows", 42LL);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const b = AddVertex(g, 42, "B");
    AddEdge(g, a, b, "likes");  // different label

    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, a}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, RelConst_NonInt64ConstValueTypesThrows) {
    graph_t p = MakeSingleVertexPattern(1, "A");
    DistanceConstraint const c = RelConst(1, "knows", std::string("42"));
    Gdd const gdd(p, Gdd::Phi{c}, Gdd::Phi{});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const b = AddVertex(g, 42, "B");
    AddEdge(g, a, b, "knows");

    auto pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, a}};

    EXPECT_THROW(gdd.Satisfies(g, map), std::logic_error);
}

TEST(GddSatisfiesConstraint, RelRel_SatisfiedWhenTargetsIntersect) {
    graph_t p = MakeTwoVertexPattern(1, 2, "A", "C");
    DistanceConstraint const c = RelRel(1, "knows", 2, "knows");
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const c_v = AddVertex(g, 2, "C");
    auto const d = AddVertex(g, 99, "D");

    AddEdge(g, a, d, "knows");
    AddEdge(g, c_v, d, "knows");

    auto const p1 = FindPatternVertex(p, 1);
    auto const p2 = FindPatternVertex(p, 2);
    std::unordered_map<vertex_t, vertex_t> const map{{p1, a}, {p2, c_v}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, RelRel_FailsWhenTargetsDoNotIntersect) {
    graph_t p = MakeTwoVertexPattern(1, 2, "A", "C");
    DistanceConstraint const c = RelRel(1, "knows", 2, "knows");
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const c_v = AddVertex(g, 2, "C");
    auto const d1 = AddVertex(g, 99, "D1");
    auto const d2 = AddVertex(g, 100, "D2");

    AddEdge(g, a, d1, "knows");
    AddEdge(g, c_v, d2, "knows");

    auto const p1 = FindPatternVertex(p, 1);
    auto const p2 = FindPatternVertex(p, 2);
    std::unordered_map<vertex_t, vertex_t> const map{{p1, a}, {p2, c_v}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

// according to the paper, relation names must be the same
// (v1 -(l1)-> v3 <-(l2)- v2 is impossible; must be v1 -(l)-> v3 <-(l)- v2)
TEST(GddSatisfiesConstraint, RelRel_FailsWhenRelationNamesDiffer) {
    graph_t p = MakeTwoVertexPattern(1, 2, "A", "C");
    DistanceConstraint const c = RelRel(1, "knows", 2, "likes");
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const c_v = AddVertex(g, 2, "C");
    auto const d = AddVertex(g, 99, "D");

    AddEdge(g, a, d, "knows");
    AddEdge(g, c_v, d, "likes");

    auto const p1 = FindPatternVertex(p, 1);
    auto const p2 = FindPatternVertex(p, 2);
    std::unordered_map<vertex_t, vertex_t> const map{{p1, a}, {p2, c_v}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, Label_Eq_SucceedsWhenEqual) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "label", std::string("Person"),
                                           DistanceMetric::kEditDistance, CmpOp::kEq, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "Person");
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, Label_Eq_FailsWhenDifferent) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "label", std::string("Person"),
                                           DistanceMetric::kEditDistance, CmpOp::kEq, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "Company");
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, Label_EditDistance_Le_SucceedsForCloseStrings) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "label", std::string("kitten"),
                                           DistanceMetric::kEditDistance, CmpOp::kLe, 3.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 1, "sitting");  // edit distance 3
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, Label_EditDistance_Le_FailsForTooStrictThreshold) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "label", std::string("kitten"),
                                           DistanceMetric::kEditDistance, CmpOp::kLe, 2.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 1, "sitting");  // distance 3
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, CustomAttr_EditDistance_Le_Succeeds) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "name", std::string("Alice"),
                                           DistanceMetric::kEditDistance, CmpOp::kLe, 1.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X", {{"name", "AlicE"}});  // edit distance 1
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, CustomAttr_EditDistance_Le_FailsWhenTooLarge) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "name", std::string("Alice"),
                                           DistanceMetric::kEditDistance, CmpOp::kLe, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X", {{"name", "AlicE"}});  // distance 1
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, CustomAttr_Missing_ReturnsFalse) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, "name", std::string("Alice"),
                                           DistanceMetric::kEditDistance, CmpOp::kEq, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X");  // no "name"
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_FALSE(gdd.Satisfies(g, map));
}

TEST(GddSatisfiesConstraint, CustomAttr_AbsDiff_WithStringAttribute) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c =
            AttrConst(1, "age", 18LL, DistanceMetric::kAbsDiff, CmpOp::kLe, 0.0);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X", {{"age", "18"}});
    auto const pv = FindPatternVertex(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    EXPECT_TRUE(gdd.Satisfies(g, map));
}

// TODO: wildcard tests once they will be implemented in code.

}  // namespace tests
