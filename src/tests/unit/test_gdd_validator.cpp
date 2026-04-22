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
        return algos::CreateAndLoadAlgorithm<algos::NaiveGddValidator>(option_map);
    }
};

TEST_P(GddValidatorCasesTest, ReturnsExpectedValidGdds) {
    ValidatorCase const& tc = GetParam();

    auto const graph_path = WriteTempDotFile(tc.graph_dot, tc.temp_file_name);
    auto const validator = CreateGddValidatorInstance(graph_path, tc.input_gdds);

    validator->Execute();
    auto const out = validator->GetResult();

    EXPECT_THAT(out, testing::UnorderedElementsAreArray(tc.expected_valid_gdds));
}

INSTANTIATE_TEST_SUITE_P(
        ValidatorCases, GddValidatorCasesTest,
        ::testing::Values(
                [] {
                    auto const g1 = MakeGddMishaLivesInAmsterdam();
                    auto const g2 = MakeGddRigaInLatvia();
                    auto const g3 = MakeGddVacuousImplicationNonexistentPerson();
                    auto const g4 = MakeGddPersonAge25ImpliesLivesInAmsterdamAsRelationConstraint();
                    auto const g5 = MakeGddLabelConstraintPersonImpliesCityLabelCloseToCity();

                    return ValidatorCase{
                            .case_name = "LargeGraphAllSatisfied",
                            .temp_file_name = "gdd_large_graph_all_good.dot",
                            .graph_dot = LargeGraphAllGoodDot(),
                            .input_gdds = {g1, g2, g3, g4, g5},
                            .expected_valid_gdds = {g1, g2, g3, g4, g5},
                    };
                }(),
                [] {
                    auto const gdd_bad = MakeGddMishaLivesInAmsterdam();
                    auto const gdd_ok1 = MakeGddRigaInLatvia();
                    auto const gdd_ok2 = MakeGddVacuousImplicationNonexistentPerson();
                    auto const gdd_ok3 =
                            MakeGddPersonAge25ImpliesLivesInAmsterdamAsRelationConstraint();
                    auto const gdd_ok4 = MakeGddLabelConstraintPersonImpliesCityLabelCloseToCity();

                    return ValidatorCase{
                            .case_name = "LargeGraphDetectsViolation",
                            .temp_file_name = "gdd_large_graph_with_violation.dot",
                            .graph_dot = LargeGraphWithViolationMishaAlsoLivesInRigaDot(),
                            .input_gdds = {gdd_bad, gdd_ok1, gdd_ok2, gdd_ok3, gdd_ok4},
                            .expected_valid_gdds = {gdd_ok1, gdd_ok2, gdd_ok3, gdd_ok4},
                    };
                }(),
                [] {
                    auto const gdd = MakeGddLabelConstraintPersonImpliesCityLabelCloseToCity();

                    return ValidatorCase{
                            .case_name = "UsesCustomAttributesAndLabel",
                            .temp_file_name = "gdd_custom_attrs_graph.dot",
                            .graph_dot = LargeGraphAllGoodDot(),
                            .input_gdds = {gdd},
                            .expected_valid_gdds = {gdd},
                    };
                }(),
                [] {
                    auto const gdd = MakeGddCompanyHqInAmsterdamNoCompanyInGraph();

                    return ValidatorCase{
                            .case_name = "EmptyMatchSetIsSatisfied",
                            .temp_file_name = "gdd_no_matches_company_city.dot",
                            .graph_dot = GraphNoMatchesForCompanyCityDot(),
                            .input_gdds = {gdd},
                            .expected_valid_gdds = {gdd},
                    };
                }(),
                [] {
                    auto const weak_gdd = MakeGddDblpWeakAuthorResolution();
                    auto const strong_gdd = MakeGddDblpStrongAuthorResolution();

                    return ValidatorCase{
                            .case_name = "DblpStrongHoldsWeakFails",
                            .temp_file_name = "gdd_dblp_like_graph.dot",
                            .graph_dot = DblpLikeGraphDot(),
                            .input_gdds = {weak_gdd, strong_gdd},
                            .expected_valid_gdds = {strong_gdd},
                    };
                }()),
        [](testing::TestParamInfo<ValidatorCase> const& info) {
            return SanitizeParamName(info.param.case_name);
        });

}  // namespace tests
