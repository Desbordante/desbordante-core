#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_validator/naive_gdd_validator.h"
#include "core/algorithms/ucc/hyucc/validator.h"
#include "core/parser/graph_parser/graph_parser.h"

namespace tests {
namespace {

using algos::NaiveGddValidator;

using model::Gdd;
using model::gdd::graph_t;

using model::gdd::detail::AttrTag;
using model::gdd::detail::CmpOp;
using model::gdd::detail::ConstValue;
using model::gdd::detail::DistanceConstraint;
using model::gdd::detail::DistanceMetric;
using model::gdd::detail::GddToken;
using model::gdd::detail::RelTag;

graph_t ReadGraphFromDot(std::string const& dot) {
    std::stringstream ss;
    ss << dot;
    return parser::graph_parser::gdd::ReadGraph(ss);
}

DistanceConstraint EqStrAttrToConst(std::size_t pattern_vid, std::string attr_name,
                                    std::string value) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = ConstValue{std::move(value)},
            .threshold = 0.0,
            .metric = DistanceMetric::kEditDistance,
            .op = CmpOp::kEq,
    };
}

DistanceConstraint EditLeStrAttrToConst(std::size_t pattern_vid, std::string attr_name,
                                        std::string value, double t) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = ConstValue{std::move(value)},
            .threshold = t,
            .metric = DistanceMetric::kEditDistance,
            .op = CmpOp::kLe,
    };
}

DistanceConstraint AbsDiffLeAttrToConst(std::size_t pattern_vid, std::string attr_name,
                                        ConstValue value, double t) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = std::move(value),
            .threshold = t,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kLe,
    };
}

DistanceConstraint RelToConst(std::size_t pattern_vid, std::string rel_name,
                              std::int64_t target_id) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, RelTag{std::move(rel_name)}},
            .rhs = ConstValue{target_id},
            .threshold = 0.0,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kEq,
    };
}

graph_t PatternPersonCity(std::string const& edge_label = "lives_in") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="Person"];
        1 [label="City"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

graph_t PatternCityCountry(std::string const& edge_label = "in_country") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="City"];
        1 [label="Country"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

graph_t PatternCompanyCity(std::string const& edge_label = "hq_in") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="Company"];
        1 [label="City"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

// GDD builders

Gdd MakeGdd_MishaLivesInAmsterdam() {
    auto pattern = PatternPersonCity("lives_in");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Misha")};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Amsterdam")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

Gdd MakeGdd_RigaInLatvia() {
    auto pattern = PatternCityCountry("in_country");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Riga")};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Latvia")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

Gdd MakeGdd_VacuousImplication_NonexistentPerson() {
    auto pattern = PatternPersonCity("lives_in");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Nonexistent")};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Nowhere")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

Gdd MakeGdd_CompanyHqInAmsterdam_NoCompanyInGraph() {
    auto pattern = PatternCompanyCity("hq_in");
    Gdd::Phi lhs{};  // empty LHS
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Amsterdam")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

Gdd MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint() {
    auto pattern = PatternPersonCity("lives_in");
    Gdd::Phi lhs{AbsDiffLeAttrToConst(0, "age", ConstValue{25LL}, 0.0)};
    Gdd::Phi rhs{RelToConst(0, "lives_in", 101)};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

Gdd MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity() {
    auto pattern = PatternPersonCity("lives_in");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Misha")};
    Gdd::Phi rhs{
            EditLeStrAttrToConst(1, "label", "Coty", 1.0)};  // "City" is distance 1 from "Coty"
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

graph_t LargeGraph_AllGood() {
    return ReadGraphFromDot(R"(digraph G {
        // Persons
        1 [label="Person", name="Misha", age="25", email="m@x"];
        2 [label="Person", name="Bob",   age="31"];
        3 [label="Person", name="Alice", age="22"];

        // Cities
        101 [label="City", name="Amsterdam", population="821752"];
        102 [label="City", name="Riga",      population="605273"];
        103 [label="City", name="Paris"];

        // Countries
        201 [label="Country", name="Netherlands"];
        202 [label="Country", name="Latvia"];
        203 [label="Country", name="France"];

        // lives_in
        1 -> 101 [label="lives_in"];
        2 -> 102 [label="lives_in"];
        3 -> 103 [label="lives_in"];

        // in_country
        101 -> 201 [label="in_country"];
        102 -> 202 [label="in_country"];
        103 -> 203 [label="in_country"];

        // noise / unrelated edges
        1 -> 2 [label="friend"];
        2 -> 3 [label="friend"];
        3 -> 1 [label="friend"];
        101 -> 102 [label="sister_city"];
        102 -> 103 [label="sister_city"];
    })");
}

graph_t LargeGraph_WithViolation_MishaAlsoLivesInRiga() {
    return ReadGraphFromDot(R"(digraph G {
        // Persons
        1 [label="Person", name="Misha", age="25", email="m@x"];
        2 [label="Person", name="Bob",   age="31"];
        3 [label="Person", name="Alice", age="22"];

        // Cities
        101 [label="City", name="Amsterdam", population="821752"];
        102 [label="City", name="Riga",      population="605273"];
        103 [label="City", name="Paris"];

        // Countries
        201 [label="Country", name="Netherlands"];
        202 [label="Country", name="Latvia"];
        203 [label="Country", name="France"];

        // lives_in (Misha has TWO)
        1 -> 101 [label="lives_in"];
        1 -> 102 [label="lives_in"]; // counterexample for "Misha -> Amsterdam"
        2 -> 102 [label="lives_in"];
        3 -> 103 [label="lives_in"];

        // in_country
        101 -> 201 [label="in_country"];
        102 -> 202 [label="in_country"];
        103 -> 203 [label="in_country"];

        // noise
        1 -> 2 [label="friend"];
        2 -> 3 [label="friend"];
        3 -> 1 [label="friend"];
    })");
}

graph_t Graph_NoMatchesForCompanyCity() {
    return ReadGraphFromDot(R"(digraph G {
        1 [label="Person", name="Misha"];
        2 [label="Person", name="Bob"];
        1 -> 2 [label="friend"];
    })");
}

}  // namespace

TEST(NaiveGddValidator_Interface, GenerateSatisfied_FiltersOnLargeGraph_AllSatisfied) {
    graph_t const graph = LargeGraph_AllGood();

    std::vector<Gdd> gdds;
    gdds.emplace_back(MakeGdd_MishaLivesInAmsterdam());                 // satisfied
    gdds.emplace_back(MakeGdd_RigaInLatvia());                          // satisfied
    gdds.emplace_back(MakeGdd_VacuousImplication_NonexistentPerson());  // satisfied (LHS false)
    gdds.emplace_back(
            MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint());      // satisfied
    gdds.emplace_back(MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity());  // satisfied

    std::unique_ptr<algos::GddValidator> const validator =
            std::make_unique<NaiveGddValidator>(graph, gdds);
    validator->LoadData();
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_TRUE(out.size() == 5);
}

TEST(NaiveGddValidator_Interface, GenerateSatisfied_FiltersOnLargeGraph_DetectsViolation) {
    graph_t const graph = LargeGraph_WithViolation_MishaAlsoLivesInRiga();

    auto const gdd_bad = MakeGdd_MishaLivesInAmsterdam();  // should be UNSAT due to counterexample
    auto const gdd_ok1 = MakeGdd_RigaInLatvia();           // still SAT
    auto const gdd_ok2 = MakeGdd_VacuousImplication_NonexistentPerson();  // SAT (LHS false)
    auto const gdd_ok3 = MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint();  // SAT
    auto const gdd_ok4 = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();        // SAT

    std::vector const gdds = {gdd_bad, gdd_ok1, gdd_ok2, gdd_ok3, gdd_ok4};

    std::unique_ptr<algos::GddValidator> const validator =
            std::make_unique<NaiveGddValidator>(graph, gdds);
    validator->LoadData();
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_TRUE(out.size() == 4);
    EXPECT_TRUE(std::ranges::find(out, gdd_ok1) != out.end());
    EXPECT_TRUE(std::ranges::find(out, gdd_ok2) != out.end());
    EXPECT_TRUE(std::ranges::find(out, gdd_ok3) != out.end());
    EXPECT_TRUE(std::ranges::find(out, gdd_ok4) != out.end());
}

TEST(NaiveGddValidator_Interface, GenerateSatisfied_UsesCustomAttributesAndLabel) {
    graph_t const graph = LargeGraph_AllGood();

    auto const gdd = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();

    std::unique_ptr<algos::GddValidator> const validator =
            std::make_unique<NaiveGddValidator>(graph, std::vector{gdd});
    validator->LoadData();
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_TRUE(out.size() == 1);
    EXPECT_TRUE(out[0] == gdd);
}

TEST(NaiveGddValidator_Interface, GenerateSatisfied_EmptyMatchSet_ShouldBeSatisfied) {
    // Semantics: if H(Q,G) is empty, then forall h in H: (LHS -> RHS) holds => satisfied.
    graph_t const graph = Graph_NoMatchesForCompanyCity();
    auto const gdd = MakeGdd_CompanyHqInAmsterdam_NoCompanyInGraph();

    std::unique_ptr<algos::GddValidator> const validator =
            std::make_unique<NaiveGddValidator>(graph, std::vector{gdd});
    validator->LoadData();
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_EQ(out, std::vector{gdd});
}

}  // namespace tests
