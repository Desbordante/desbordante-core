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

using model::gdd::detail::AttrTag;
using model::gdd::detail::CmpOp;
using model::gdd::detail::DistanceConstraint;
using model::gdd::detail::DistanceMetric;
using model::gdd::detail::GddToken;

model::gdd::vertex_t AddVertex(graph_t& g, int id, std::string label,
                               std::unordered_map<std::string, std::string> attrs = {}) {
    auto const v = boost::add_vertex(g);
    g[v].id = id;
    g[v].label = std::move(label);
    g[v].attributes = std::move(attrs);
    return v;
}

void AddEdge(graph_t& g, model::gdd::vertex_t from, model::gdd::vertex_t to, std::string label) {
    auto [e, ok] = boost::add_edge(from, to, g);
    ASSERT_TRUE(ok);
    g[e].label = std::move(label);
}

graph_t MakePersonCityPattern(bool reverse_insertion_order, int id_shift,
                              std::string edge_label = "lives_in",
                              std::string person_name = "Misha",
                              std::string city_name = "Amsterdam") {
    graph_t g;

    model::gdd::vertex_t v_person{};
    model::gdd::vertex_t v_city{};

    if (!reverse_insertion_order) {
        v_person = AddVertex(g, 1 + id_shift, "Person", {{"name", person_name}});
        v_city = AddVertex(g, 2 + id_shift, "City", {{"name", city_name}});
    } else {
        v_city = AddVertex(g, 2 + id_shift, "City", {{"name", city_name}});
        v_person = AddVertex(g, 1 + id_shift, "Person", {{"name", person_name}});
    }

    AddEdge(g, v_person, v_city, std::move(edge_label));
    return g;
}

DistanceConstraint MakeAttrEditDistanceLe(size_t pattern_vid, std::string attr_name,
                                          std::string constant, double threshold) {
    return DistanceConstraint{.lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
                              .rhs = model::gdd::detail::ConstValue{std::move(constant)},
                              .threshold = threshold,
                              .metric = DistanceMetric::kEditDistance,
                              .op = CmpOp::kLe};
}

}  // namespace

TEST(GddEquality, Reflexive) {
    graph_t const pattern = MakePersonCityPattern(false, 0);
    Gdd const g(pattern, Gdd::Phi{}, Gdd::Phi{});
    EXPECT_TRUE(g == g);
}

TEST(GddEquality, IsomorphicPatternsEqualEvenIfVertexIdsDiffer) {
    graph_t const p1 = MakePersonCityPattern(false, 0);
    graph_t const p2 = MakePersonCityPattern(true, 100);

    Gdd::Phi const lhs{MakeAttrEditDistanceLe(0, "name", "Alice", 0.0)};
    Gdd::Phi const rhs{MakeAttrEditDistanceLe(1, "name", "Riga", 0.0)};

    Gdd const g1(p1, lhs, rhs);
    Gdd const g2(p2, lhs, rhs);

    EXPECT_TRUE(g1 == g2);
    EXPECT_TRUE(g2 == g1);
}

TEST(GddEquality, DifferentVertexLabelNotEqual) {
    graph_t const p1 = MakePersonCityPattern(false, 0);
    graph_t p2 = MakePersonCityPattern(false, 0);

    auto const v0 = *vertices(p2).first;
    p2[v0].label = "Company";

    Gdd const g1(p1, Gdd::Phi{}, Gdd::Phi{});
    Gdd const g2(p2, Gdd::Phi{}, Gdd::Phi{});
    EXPECT_FALSE(g1 == g2);
    EXPECT_FALSE(g2 == g1);
}

TEST(GddEquality, DifferentVertexAttributesNotEqual) {
    graph_t const p1 = MakePersonCityPattern(false, 0, "lives_in", "Alice", "Riga");
    graph_t const p2 = MakePersonCityPattern(false, 0, "lives_in", "Bob", "Riga");

    Gdd const g1(p1, Gdd::Phi{}, Gdd::Phi{});
    Gdd const g2(p2, Gdd::Phi{}, Gdd::Phi{});
    EXPECT_FALSE(g1 == g2);
    EXPECT_FALSE(g2 == g1);
}

TEST(GddEquality, DifferentEdgeLabelNotEqual) {
    graph_t const p1 = MakePersonCityPattern(false, 0, "lives_in");
    graph_t const p2 = MakePersonCityPattern(false, 0, "works_in");

    Gdd const g1(p1, Gdd::Phi{}, Gdd::Phi{});
    Gdd const g2(p2, Gdd::Phi{}, Gdd::Phi{});
    EXPECT_FALSE(g1 == g2);
    EXPECT_FALSE(g2 == g1);
}

TEST(GddEquality, DifferentConstraintsNotEqual) {
    graph_t p = MakePersonCityPattern(false, 0);

    DistanceConstraint const c1 = MakeAttrEditDistanceLe(0, "name", "Misha", 0.0);
    DistanceConstraint c2 = c1;
    c2.metric = DistanceMetric::kAbsDiff;

    Gdd const g1(p, Gdd::Phi{c1}, Gdd::Phi{});
    Gdd const g2(p, Gdd::Phi{c2}, Gdd::Phi{});
    EXPECT_FALSE(g1 == g2);
    EXPECT_FALSE(g2 == g1);
}

TEST(GddEquality, ConstraintOrderMattersWithVectorEquality) {
    graph_t const p = MakePersonCityPattern(false, 0);

    DistanceConstraint const c1 = MakeAttrEditDistanceLe(0, "name", "Alice", 0.0);
    DistanceConstraint const c2 = MakeAttrEditDistanceLe(1, "name", "Riga", 0.0);

    Gdd const g1(p, Gdd::Phi{c1, c2}, Gdd::Phi{});
    Gdd const g2(p, Gdd::Phi{c2, c1}, Gdd::Phi{});
    EXPECT_FALSE(g1 == g2);
    EXPECT_FALSE(g2 == g1);
}

TEST(GddEquality, ConstraintThresholdComparisonShouldBeSymmetric) {
    graph_t p = MakePersonCityPattern(false, 0);

    DistanceConstraint const smaller = MakeAttrEditDistanceLe(0, "name", "Alice", 1.0);
    DistanceConstraint const larger = MakeAttrEditDistanceLe(0, "name", "Alice", 1.0 + 1e-3);

    Gdd const a(p, Gdd::Phi{smaller}, Gdd::Phi{});
    Gdd const b(p, Gdd::Phi{larger}, Gdd::Phi{});

    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

}  // namespace tests
