#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_validator/naive_gdd_validator.h"
#include "core/config/names.h"
#include "tests/unit/test_gdd_utils.h"

namespace tests {

namespace {

using algos::NaiveGddValidator;

using model::Gdd;
using model::gdd::graph_t;

using ::testing::TestParamInfo;
using ::testing::UnorderedElementsAreArray;

std::string SanitizeParamName(std::string name) {
    for (char& ch : name) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            ch = '_';
        }
    }
    return name;
}

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
    Gdd::Phi lhs{AbsDiffLeAttrToConst(0, "age", model::gdd::detail::ConstValue{25LL}, 0.0)};
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

std::string DblpLikeGraph_Dot() {
    return R"(digraph G {
        1 [label="Author", name="Jiawei Han", canonical_author_id="author:han_jiawei"];
        2 [label="Author", name="J. Han",     canonical_author_id="author:han_jiawei"];

        3 [label="Author", name="Philip S. Yu", canonical_author_id="author:yu_philip"];

        4 [label="Author", name="Yi Zhang", canonical_author_id="author:zhang_yi"];
        5 [label="Author", name="Yu Zhang", canonical_author_id="author:zhang_yu"];

        101 [label="Paper", title="Mining Frequent Patterns",     year="2000"];
        102 [label="Paper", title="Mining Frequent Pattern Sets", year="2000"];
        103 [label="Paper", title="Scalable Pattern Search",      year="2023"];
        104 [label="Paper", title="Efficient Pattern Search",     year="2023"];

        201 [label="Venue", name="SIGMOD"];
        202 [label="Venue", name="KDD"];

        1 -> 101 [label="authored"];
        3 -> 101 [label="authored"];

        2 -> 102 [label="authored"];
        3 -> 102 [label="authored"];

        4 -> 103 [label="authored"];
        5 -> 104 [label="authored"];

        101 -> 201 [label="published_in"];
        102 -> 201 [label="published_in"];
        103 -> 202 [label="published_in"];
        104 -> 202 [label="published_in"];
    })";
}

graph_t PatternDblpStrong() {
    return ReadGraphFromDot(R"(digraph P {
        0 [label="Author"];
        1 [label="Author"];
        2 [label="Paper"];
        3 [label="Paper"];
        4 [label="Author"];
        5 [label="Venue"];

        0 -> 2 [label="authored"];
        1 -> 3 [label="authored"];
        4 -> 2 [label="authored"];
        4 -> 3 [label="authored"];
        2 -> 5 [label="published_in"];
        3 -> 5 [label="published_in"];
    })");
}

graph_t PatternDblpWeak() {
    return ReadGraphFromDot(R"(digraph P {
        0 [label="Author"];
        1 [label="Author"];
        2 [label="Paper"];
        3 [label="Paper"];
        4 [label="Venue"];

        0 -> 2 [label="authored"];
        1 -> 3 [label="authored"];
        2 -> 4 [label="published_in"];
        3 -> 4 [label="published_in"];
    })");
}

Gdd MakeGdd_DblpStrongAuthorResolution() {
    auto pattern = PatternDblpStrong();
    Gdd::Phi lhs{
            EditLeAttrToAttr(0, "name", 1, "name", 8.0),
            EditLeAttrToAttr(2, "year", 3, "year", 0.0),
    };
    Gdd::Phi rhs{
            EditLeAttrToAttr(0, "canonical_author_id", 1, "canonical_author_id", 0.0),
    };
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

Gdd MakeGdd_DblpWeakAuthorResolution() {
    auto pattern = PatternDblpWeak();
    Gdd::Phi lhs{
            EditLeAttrToAttr(0, "name", 1, "name", 2.0),
            EditLeAttrToAttr(2, "year", 3, "year", 0.0),
    };
    Gdd::Phi rhs{
            EditLeAttrToAttr(0, "canonical_author_id", 1, "canonical_author_id", 0.0),
    };
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

}  // namespace

struct ValidatorCase {
    std::string case_name;
    std::string temp_file_name;
    std::string graph_dot;
    std::vector<Gdd> input_gdds;
    std::vector<Gdd> expected_valid_gdds;
};

class GddValidatorCasesTest : public ::testing::TestWithParam<ValidatorCase> {
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
            std::filesystem::path const& graph_path, std::vector<Gdd> const& gdds) {
        algos::StdParamsMap const option_map = {
                {config::names::kGraphData, graph_path},
                {config::names::kGddData, gdds},
        };
        return algos::CreateAndLoadAlgorithm<NaiveGddValidator>(option_map);
    }
};

TEST_P(GddValidatorCasesTest, ReturnsExpectedValidGdds) {
    ValidatorCase const& tc = GetParam();

    auto const graph_path = WriteTempDotFile(tc.graph_dot, tc.temp_file_name);
    auto const validator = CreateGddValidatorInstance(graph_path, tc.input_gdds);

    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_THAT(out, UnorderedElementsAreArray(tc.expected_valid_gdds));
}

INSTANTIATE_TEST_SUITE_P(
        ValidatorCases, GddValidatorCasesTest,
        ::testing::Values(
                [] {
                    auto const g1 = MakeGdd_MishaLivesInAmsterdam();
                    auto const g2 = MakeGdd_RigaInLatvia();
                    auto const g3 = MakeGdd_VacuousImplication_NonexistentPerson();
                    auto const g4 = MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint();
                    auto const g5 = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();

                    return ValidatorCase{
                            .case_name = "LargeGraphAllSatisfied",
                            .temp_file_name = "gdd_large_graph_all_good.dot",
                            .graph_dot = LargeGraph_AllGood_Dot(),
                            .input_gdds = {g1, g2, g3, g4, g5},
                            .expected_valid_gdds = {g1, g2, g3, g4, g5},
                    };
                }(),
                [] {
                    auto const gdd_bad = MakeGdd_MishaLivesInAmsterdam();
                    auto const gdd_ok1 = MakeGdd_RigaInLatvia();
                    auto const gdd_ok2 = MakeGdd_VacuousImplication_NonexistentPerson();
                    auto const gdd_ok3 =
                            MakeGdd_PersonAge25ImpliesLivesInAmsterdam_AsRelationConstraint();
                    auto const gdd_ok4 = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();

                    return ValidatorCase{
                            .case_name = "LargeGraphDetectsViolation",
                            .temp_file_name = "gdd_large_graph_with_violation.dot",
                            .graph_dot = LargeGraph_WithViolation_MishaAlsoLivesInRiga_Dot(),
                            .input_gdds = {gdd_bad, gdd_ok1, gdd_ok2, gdd_ok3, gdd_ok4},
                            .expected_valid_gdds = {gdd_ok1, gdd_ok2, gdd_ok3, gdd_ok4},
                    };
                }(),
                [] {
                    auto const gdd = MakeGdd_LabelConstraint_PersonImpliesCityLabelCloseToCity();

                    return ValidatorCase{
                            .case_name = "UsesCustomAttributesAndLabel",
                            .temp_file_name = "gdd_custom_attrs_graph.dot",
                            .graph_dot = LargeGraph_AllGood_Dot(),
                            .input_gdds = {gdd},
                            .expected_valid_gdds = {gdd},
                    };
                }(),
                [] {
                    auto const gdd = MakeGdd_CompanyHqInAmsterdam_NoCompanyInGraph();

                    return ValidatorCase{
                            .case_name = "EmptyMatchSetIsSatisfied",
                            .temp_file_name = "gdd_no_matches_company_city.dot",
                            .graph_dot = Graph_NoMatchesForCompanyCity_Dot(),
                            .input_gdds = {gdd},
                            .expected_valid_gdds = {gdd},
                    };
                }(),
                [] {
                    auto const weak_gdd = MakeGdd_DblpWeakAuthorResolution();
                    auto const strong_gdd = MakeGdd_DblpStrongAuthorResolution();

                    return ValidatorCase{
                            .case_name = "DblpStrongHoldsWeakFails",
                            .temp_file_name = "gdd_dblp_like_graph.dot",
                            .graph_dot = DblpLikeGraph_Dot(),
                            .input_gdds = {weak_gdd, strong_gdd},
                            .expected_valid_gdds = {strong_gdd},
                    };
                }()),
        [](TestParamInfo<ValidatorCase> const& info) {
            return SanitizeParamName(info.param.case_name);
        });

}  // namespace tests
