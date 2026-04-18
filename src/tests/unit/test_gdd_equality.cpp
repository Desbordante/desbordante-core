#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "test_gdd_utils.h"

namespace tests {

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
