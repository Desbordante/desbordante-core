#include <cctype>
#include <string>
#include <unordered_map>
#include <utility>

#include <gtest/gtest.h>

#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "tests/unit/test_gdd_utils.h"

// TODO: wildcard tests once they will be implemented in code.

using model::Gdd;
using model::gdd::graph_t;
using model::gdd::vertex_t;

using model::gdd::detail::CmpOp;
using model::gdd::detail::ConstValue;
using model::gdd::detail::DistanceConstraint;
using model::gdd::detail::DistanceMetric;

using namespace tests::gdd::utils;

namespace tests {

struct AttrConstCase {
    std::string case_name;
    std::uint64_t graph_vertex_id;
    std::string graph_vertex_label;
    std::unordered_map<std::string, std::string> graph_vertex_attrs;
    std::string attr_name;
    ConstValue rhs;
    DistanceMetric metric;
    CmpOp op;
    double threshold;
    bool expected;
    bool expect_throw = false;
};

class GddSatisfiesAttrConstTest : public ::testing::TestWithParam<AttrConstCase> {};

TEST_P(GddSatisfiesAttrConstTest, WorksAsExpected) {
    AttrConstCase const& tc = GetParam();

    graph_t p = MakeSingleVertexPattern(1, "X");
    DistanceConstraint const c = AttrConst(1, tc.attr_name, tc.rhs, tc.metric, tc.op, tc.threshold);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const gv = AddVertex(g, tc.graph_vertex_id, tc.graph_vertex_label, tc.graph_vertex_attrs);
    auto const pv = FindVertexById(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, gv}};

    if (tc.expect_throw) {
        EXPECT_THROW(gdd.Satisfies(g, map), std::logic_error);
    } else {
        EXPECT_EQ(gdd.Satisfies(g, map), tc.expected);
    }
}

INSTANTIATE_TEST_SUITE_P(
        AttrConstCases, GddSatisfiesAttrConstTest,
        ::testing::Values(
                AttrConstCase{
                        .case_name = "AbsDiffUsesNumericParsing",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {},
                        .attr_name = "id",
                        .rhs = ConstValue{10LL},
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kLe,
                        .threshold = 0.0,
                        .expected = true,
                },
                AttrConstCase{
                        .case_name = "AbsDiffFailsWhenBeyondThreshold",
                        .graph_vertex_id = 11,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {},
                        .attr_name = "id",
                        .rhs = ConstValue{10LL},
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kLe,
                        .threshold = 0.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "AbsDiffAllowsDistanceWithinThreshold",
                        .graph_vertex_id = 11,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {},
                        .attr_name = "id",
                        .rhs = ConstValue{10LL},
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kLe,
                        .threshold = 1.0,
                        .expected = true,
                },
                AttrConstCase{
                        .case_name = "EditDistanceStrings",
                        .graph_vertex_id = 1,
                        .graph_vertex_label = "sitting",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("kitten")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kLe,
                        .threshold = 3.0,
                        .expected = true,
                },
                AttrConstCase{
                        .case_name = "EditDistanceFailsWhenTooLarge",
                        .graph_vertex_id = 1,
                        .graph_vertex_label = "sitting",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("kitten")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kLe,
                        .threshold = 2.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "AttrMissingInGraphReturnsFalse",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {},
                        .attr_name = "color",
                        .rhs = ConstValue{0LL},
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kLe,
                        .threshold = 0.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "AbsDiffWithStringConstThrows",
                        .graph_vertex_id = 1,
                        .graph_vertex_label = "abc",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("abc")},
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kLe,
                        .threshold = 0.0,
                        .expected = false,
                        .expect_throw = true,
                },
                AttrConstCase{
                        .case_name = "LabelEqSucceedsWhenEqual",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "Person",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("Person")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kEq,
                        .threshold = 0.0,
                        .expected = true,
                },
                AttrConstCase{
                        .case_name = "LabelEqFailsWhenDifferent",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "Company",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("Person")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kEq,
                        .threshold = 0.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "LabelEditDistanceLeSucceedsForCloseStrings",
                        .graph_vertex_id = 1,
                        .graph_vertex_label = "sitting",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("kitten")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kLe,
                        .threshold = 3.0,
                        .expected = true,
                },
                AttrConstCase{
                        .case_name = "LabelEditDistanceLeFailsForTooStrictThreshold",
                        .graph_vertex_id = 1,
                        .graph_vertex_label = "sitting",
                        .graph_vertex_attrs = {},
                        .attr_name = "label",
                        .rhs = ConstValue{std::string("kitten")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kLe,
                        .threshold = 2.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "CustomAttrEditDistanceLeSucceeds",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {{"name", "AlicE"}},
                        .attr_name = "name",
                        .rhs = ConstValue{std::string("Alice")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kLe,
                        .threshold = 1.0,
                        .expected = true,
                },
                AttrConstCase{
                        .case_name = "CustomAttrEditDistanceLeFailsWhenTooLarge",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {{"name", "AlicE"}},
                        .attr_name = "name",
                        .rhs = ConstValue{std::string("Alice")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kLe,
                        .threshold = 0.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "CustomAttrMissingReturnsFalse",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {},
                        .attr_name = "name",
                        .rhs = ConstValue{std::string("Alice")},
                        .metric = DistanceMetric::kEditDistance,
                        .op = CmpOp::kEq,
                        .threshold = 0.0,
                        .expected = false,
                },
                AttrConstCase{
                        .case_name = "CustomAttrAbsDiffWithStringAttribute",
                        .graph_vertex_id = 10,
                        .graph_vertex_label = "X",
                        .graph_vertex_attrs = {{"age", "18"}},
                        .attr_name = "age",
                        .rhs = ConstValue{18LL},
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kLe,
                        .threshold = 0.0,
                        .expected = true,
                }),
        [](testing::TestParamInfo<AttrConstCase> const& info) {
            return SanitizeParamName(info.param.case_name);
        });

struct AttrAttrCase {
    std::string case_name;
    std::uint64_t lhs_graph_id;
    std::uint64_t rhs_graph_id;
    std::string lhs_graph_label;
    std::string rhs_graph_label;
    std::string lhs_attr_name;
    std::string rhs_attr_name;
    DistanceMetric metric;
    CmpOp op;
    double threshold;
    bool expected;
};

class GddSatisfiesAttrAttrTest : public ::testing::TestWithParam<AttrAttrCase> {};

TEST_P(GddSatisfiesAttrAttrTest, WorksAsExpected) {
    AttrAttrCase const& tc = GetParam();

    graph_t p = MakeTwoVertexPattern(1, 2, "A", "B");
    DistanceConstraint const c =
            AttrAttr(1, tc.lhs_attr_name, 2, tc.rhs_attr_name, tc.metric, tc.op, tc.threshold);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const g1 = AddVertex(g, tc.lhs_graph_id, tc.lhs_graph_label);
    auto const g2 = AddVertex(g, tc.rhs_graph_id, tc.rhs_graph_label);
    auto const p1 = FindVertexById(p, 1);
    auto const p2 = FindVertexById(p, 2);

    std::unordered_map<vertex_t, vertex_t> const map{{p1, g1}, {p2, g2}};
    EXPECT_EQ(gdd.Satisfies(g, map), tc.expected);
}

INSTANTIATE_TEST_SUITE_P(AttrAttrCases, GddSatisfiesAttrAttrTest,
                         ::testing::Values(
                                 AttrAttrCase{
                                         .case_name = "AbsDiffBetweenTwoMappedVertices",
                                         .lhs_graph_id = 10,
                                         .rhs_graph_id = 12,
                                         .lhs_graph_label = "A",
                                         .rhs_graph_label = "B",
                                         .lhs_attr_name = "id",
                                         .rhs_attr_name = "id",
                                         .metric = DistanceMetric::kAbsDiff,
                                         .op = CmpOp::kLe,
                                         .threshold = 2.0,
                                         .expected = true,
                                 },
                                 AttrAttrCase{
                                         .case_name = "AbsDiffFailsBetweenTwoMappedVertices",
                                         .lhs_graph_id = 10,
                                         .rhs_graph_id = 12,
                                         .lhs_graph_label = "A",
                                         .rhs_graph_label = "B",
                                         .lhs_attr_name = "id",
                                         .rhs_attr_name = "id",
                                         .metric = DistanceMetric::kAbsDiff,
                                         .op = CmpOp::kLe,
                                         .threshold = 1.0,
                                         .expected = false,
                                 }),
                         [](testing::TestParamInfo<AttrAttrCase> const& info) {
                             return SanitizeParamName(info.param.case_name);
                         });

struct RelConstCase {
    std::string case_name;
    std::string actual_edge_label;
    bool expected;
};

class GddSatisfiesRelConstTest : public ::testing::TestWithParam<RelConstCase> {};

TEST_P(GddSatisfiesRelConstTest, WorksAsExpected) {
    RelConstCase const& tc = GetParam();

    graph_t p = MakeSingleVertexPattern(1, "A");
    DistanceConstraint const c = RelConst(1, "knows", 42LL);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const b = AddVertex(g, 42, "B");
    AddEdge(g, a, b, tc.actual_edge_label);

    auto const pv = FindVertexById(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, a}};

    EXPECT_EQ(gdd.Satisfies(g, map), tc.expected);
}

INSTANTIATE_TEST_SUITE_P(RelConstCases, GddSatisfiesRelConstTest,
                         ::testing::Values(
                                 RelConstCase{
                                         .case_name = "SatisfiedWhenEdgeEndsAtCr",
                                         .actual_edge_label = "knows",
                                         .expected = true,
                                 },
                                 RelConstCase{
                                         .case_name = "FailsWhenNoSuchEdge",
                                         .actual_edge_label = "likes",
                                         .expected = false,
                                 }),
                         [](testing::TestParamInfo<RelConstCase> const& info) {
                             return SanitizeParamName(info.param.case_name);
                         });

struct RelRelCase {
    std::string case_name;
    std::string lhs_rel_name;
    std::string rhs_rel_name;
    std::string lhs_edge_label;
    std::string rhs_edge_label;
    bool use_shared_target;
    bool expected;
};

class GddSatisfiesRelRelTest : public ::testing::TestWithParam<RelRelCase> {};

TEST_P(GddSatisfiesRelRelTest, WorksAsExpected) {
    RelRelCase const& tc = GetParam();

    graph_t p = MakeTwoVertexPattern(1, 2, "A", "C");
    DistanceConstraint const c = RelRel(1, tc.lhs_rel_name, 2, tc.rhs_rel_name);
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{c});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const c_v = AddVertex(g, 2, "C");

    if (tc.use_shared_target) {
        auto const d = AddVertex(g, 99, "D");
        AddEdge(g, a, d, tc.lhs_edge_label);
        AddEdge(g, c_v, d, tc.rhs_edge_label);
    } else {
        auto const d1 = AddVertex(g, 99, "D1");
        auto const d2 = AddVertex(g, 100, "D2");
        AddEdge(g, a, d1, tc.lhs_edge_label);
        AddEdge(g, c_v, d2, tc.rhs_edge_label);
    }

    auto const p1 = FindVertexById(p, 1);
    auto const p2 = FindVertexById(p, 2);
    std::unordered_map<vertex_t, vertex_t> const map{{p1, a}, {p2, c_v}};

    EXPECT_EQ(gdd.Satisfies(g, map), tc.expected);
}

INSTANTIATE_TEST_SUITE_P(RelRelCases, GddSatisfiesRelRelTest,
                         ::testing::Values(
                                 RelRelCase{
                                         .case_name = "SatisfiedWhenTargetsIntersect",
                                         .lhs_rel_name = "knows",
                                         .rhs_rel_name = "knows",
                                         .lhs_edge_label = "knows",
                                         .rhs_edge_label = "knows",
                                         .use_shared_target = true,
                                         .expected = true,
                                 },
                                 RelRelCase{
                                         .case_name = "FailsWhenTargetsDoNotIntersect",
                                         .lhs_rel_name = "knows",
                                         .rhs_rel_name = "knows",
                                         .lhs_edge_label = "knows",
                                         .rhs_edge_label = "knows",
                                         .use_shared_target = false,
                                         .expected = false,
                                 },
                                 RelRelCase{
                                         .case_name = "FailsWhenRelationNamesDiffer",
                                         .lhs_rel_name = "knows",
                                         .rhs_rel_name = "likes",
                                         .lhs_edge_label = "knows",
                                         .rhs_edge_label = "likes",
                                         .use_shared_target = true,
                                         .expected = false,
                                 }),
                         [](testing::TestParamInfo<RelRelCase> const& info) {
                             return SanitizeParamName(info.param.case_name);
                         });

TEST(GddSatisfiesConstraint, EmptyLhsRhsSatisfies) {
    graph_t p = MakeSingleVertexPattern(1, "X");
    Gdd const gdd(p, Gdd::Phi{}, Gdd::Phi{});

    graph_t g;
    auto const gv = AddVertex(g, 10, "X");
    auto const pv = FindVertexById(p, 1);

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

TEST(GddSatisfiesConstraint, RelConstNonInt64ConstValueTypesThrows) {
    graph_t p = MakeSingleVertexPattern(1, "A");
    DistanceConstraint const c = RelConst(1, "knows", std::string("42"));
    Gdd const gdd(p, Gdd::Phi{c}, Gdd::Phi{});

    graph_t g;
    auto const a = AddVertex(g, 1, "A");
    auto const b = AddVertex(g, 42, "B");
    AddEdge(g, a, b, "knows");

    auto const pv = FindVertexById(p, 1);
    std::unordered_map<vertex_t, vertex_t> const map{{pv, a}};

    EXPECT_THROW(gdd.Satisfies(g, map), std::logic_error);
}

}  // namespace tests