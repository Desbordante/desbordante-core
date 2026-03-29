#include <filesystem>
#include <fstream>
#include <memory>
#include <ranges>
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
#include "core/config/names.h"
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
    Gdd::Phi lhs{};
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
    Gdd::Phi rhs{EditLeStrAttrToConst(1, "label", "Coty", 1.0)};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

std::string LargeGraph_AllGood_Dot() {
    return R"(digraph G {
        1 [label="Person", name="Misha", age="25", email="m@x"];
        2 [label="Person", name="Bob",   age="31"];
        3 [label="Person", name="Alice", age="22"];

        101 [label="City", name="Amsterdam", population="821752"];
        102 [label="City", name="Riga",      population="605273"];
        103 [label="City", name="Paris"];

        201 [label="Country", name="Netherlands"];
        202 [label="Country", name="Latvia"];
        203 [label="Country", name="France"];

        1 -> 101 [label="lives_in"];
        2 -> 102 [label="lives_in"];
        3 -> 103 [label="lives_in"];

        101 -> 201 [label="in_country"];
        102 -> 202 [label="in_country"];
        103 -> 203 [label="in_country"];

        1 -> 2 [label="friend"];
        2 -> 3 [label="friend"];
        3 -> 1 [label="friend"];
        101 -> 102 [label="sister_city"];
        102 -> 103 [label="sister_city"];
    })";
}

std::string LargeGraph_WithViolation_MishaAlsoLivesInRiga_Dot() {
    return R"(digraph G {
        1 [label="Person", name="Misha", age="25", email="m@x"];
        2 [label="Person", name="Bob",   age="31"];
        3 [label="Person", name="Alice", age="22"];

        101 [label="City", name="Amsterdam", population="821752"];
        102 [label="City", name="Riga",      population="605273"];
        103 [label="City", name="Paris"];

        201 [label="Country", name="Netherlands"];
        202 [label="Country", name="Latvia"];
        203 [label="Country", name="France"];

        1 -> 101 [label="lives_in"];
        1 -> 102 [label="lives_in"];
        2 -> 102 [label="lives_in"];
        3 -> 103 [label="lives_in"];

        101 -> 201 [label="in_country"];
        102 -> 202 [label="in_country"];
        103 -> 203 [label="in_country"];

        1 -> 2 [label="friend"];
        2 -> 3 [label="friend"];
        3 -> 1 [label="friend"];
    })";
}

std::string Graph_NoMatchesForCompanyCity_Dot() {
    return R"(digraph G {
        1 [label="Person", name="Misha"];
        2 [label="Person", name="Bob"];
        1 -> 2 [label="friend"];
    })";
}

}  // namespace

class GddValidatorTest : public ::testing::Test {
protected:
    static std::filesystem::path WriteTempDotFile(std::string const& dot,
                                                  std::string const& file_name) {
        auto const path = std::filesystem::temp_directory_path() / file_name;
        std::ofstream out(path);
        out << dot;
        out.close();
        return path;
    }

    static std::unique_ptr<algos::GddValidator> CreateGddValidatorInstance(
            std::filesystem::path const& graph_path, std::vector<model::Gdd> const& gdds) {
        algos::StdParamsMap const option_map = {
                {config::names::kGraphData, graph_path},
                {config::names::kGddData, gdds},
        };
        return algos::CreateAndLoadAlgorithm<algos::NaiveGddValidator>(option_map);
    }
};

TEST_F(GddValidatorTest, GenerateSatisfied_FiltersOnLargeGraph_AllSatisfied) {
    auto const graph_path =
            WriteTempDotFile(LargeGraph_AllGood_Dot(), "gdd_large_graph_all_good.dot");

    std::vector<Gdd> gdds;
    gdds.emplace_back(MakeGdd_MishaLivesInAmsterdam());
    gdds.emplace_back(MakeGdd_RigaInLatvia());
    gdds.emplace_back(MakeGdd_VacuousImplication_NonexistentPerson());
    gdds.emplace_back(MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint());
    gdds.emplace_back(MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity());

    auto const validator = CreateGddValidatorInstance(graph_path, gdds);
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_EQ(out.size(), 5);
}

TEST_F(GddValidatorTest, GenerateSatisfied_FiltersOnLargeGraph_DetectsViolation) {
    auto const graph_path = WriteTempDotFile(LargeGraph_WithViolation_MishaAlsoLivesInRiga_Dot(),
                                             "gdd_large_graph_with_violation.dot");

    auto const gdd_bad = MakeGdd_MishaLivesInAmsterdam();
    auto const gdd_ok1 = MakeGdd_RigaInLatvia();
    auto const gdd_ok2 = MakeGdd_VacuousImplication_NonexistentPerson();
    auto const gdd_ok3 = MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint();
    auto const gdd_ok4 = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();

    std::vector const gdds = {gdd_bad, gdd_ok1, gdd_ok2, gdd_ok3, gdd_ok4};

    auto validator = CreateGddValidatorInstance(graph_path, gdds);
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_EQ(out.size(), 4);
    EXPECT_TRUE(std::ranges::find(out, gdd_ok1) != out.end());
    EXPECT_TRUE(std::ranges::find(out, gdd_ok2) != out.end());
    EXPECT_TRUE(std::ranges::find(out, gdd_ok3) != out.end());
    EXPECT_TRUE(std::ranges::find(out, gdd_ok4) != out.end());
}

TEST_F(GddValidatorTest, GenerateSatisfied_UsesCustomAttributesAndLabel) {
    auto const graph_path =
            WriteTempDotFile(LargeGraph_AllGood_Dot(), "gdd_custom_attrs_graph.dot");

    auto const gdd = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();

    auto const validator = CreateGddValidatorInstance(graph_path, std::vector{gdd});
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], gdd);
}

TEST_F(GddValidatorTest, GenerateSatisfied_EmptyMatchSet_ShouldBeSatisfied) {
    auto const graph_path = WriteTempDotFile(Graph_NoMatchesForCompanyCity_Dot(),
                                             "gdd_no_matches_company_city.dot");
    auto const gdd = MakeGdd_CompanyHqInAmsterdam_NoCompanyInGraph();

    auto const validator = CreateGddValidatorInstance(graph_path, std::vector{gdd});
    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_EQ(out, std::vector{gdd});
}

}  // namespace tests
