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
#include "core/algorithms/gdd/gdd_validator/wcoj_gdd_validator.h"
#include "core/config/names.h"
#include "core/config/thread_number/type.h"
#include "tests/unit/test_gdd_utils.h"

using model::Gdd;

using tests::gdd::utils::AttrAttr;
using tests::gdd::utils::AttrConst;
using tests::gdd::utils::EqStrAttrToConst;
using tests::gdd::utils::MakeArrowPattern;
using tests::gdd::utils::MakePatternPersonCity;
using tests::gdd::utils::ReadGraphFromDot;

using tests::gdd::utils::SanitizeParamName;

using tests::gdd::utils::MakeGddCompanyHqInAmsterdamNoCompanyInGraph;
using tests::gdd::utils::MakeGddDblpStrongAuthorResolution;
using tests::gdd::utils::MakeGddDblpWeakAuthorResolution;
using tests::gdd::utils::MakeGddLabelConstraintPersonImpliesCityLabelCloseToCity;
using tests::gdd::utils::MakeGddMishaLivesInAmsterdam;
using tests::gdd::utils::MakeGddPersonAge25ImpliesLivesInAmsterdamAsRelationConstraint;
using tests::gdd::utils::MakeGddRigaInLatvia;
using tests::gdd::utils::MakeGddVacuousImplicationNonexistentPerson;

using tests::gdd::utils::DblpLikeGraphDot;
using tests::gdd::utils::LargeGraphAllGoodDot;
using tests::gdd::utils::LargeGraphWithViolationMishaAlsoLivesInRigaDot;

namespace tests {

struct ValidatorCase {
    std::string case_name;
    std::string temp_file_name;
    std::string graph_dot;
    std::vector<Gdd> input_gdds;
    std::vector<Gdd> expected_valid_gdds;
    std::vector<std::size_t> expected_counterexample_gdd_indices{};
};

std::vector<ValidatorCase> GetValidatorCases() {
    return {[] {
                return ValidatorCase{
                        .case_name = "EmptyGddInput",
                        .temp_file_name = "gdd_empty_input.dot",
                        .graph_dot = R"(digraph G {
                            1 [label = "X"];
                        })",
                        .input_gdds = {},
                        .expected_valid_gdds = {},
                };
            }(),
            [] {
                auto const pattern = tests::gdd::utils::MakeSingleVertexPattern(0, "City");

                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(0, "name", "Impossible")});
                return ValidatorCase{
                        .case_name = "VertexLabelMatters",
                        .temp_file_name = "gdd_vertex_label_matters.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "Country", name = "France"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {gdd},
                };
            }(),
            [] {
                auto const pattern = MakePatternPersonCity("lives_in");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(1, "name", "Impossible")});
                return ValidatorCase{
                        .case_name = "EdgeLabelMatters",
                        .temp_file_name = "gdd_edge_label_matters.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "Person", name = "Misha"];
                                2 [label = "City", name = "Amsterdam"];
                                1 -> 2 [label = "works_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {gdd},
                };
            }(),
            [] {
                auto const pattern = MakePatternPersonCity();
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(1, "label", "Impossible")});
                return ValidatorCase{
                        .case_name = "DirectedEdgeOrientationMatters",
                        .temp_file_name = "gdd_directed_edge_orientation_matters.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "Person"];
                                2 [label = "City"];
                                2 -> 1 [label = "lives_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {gdd},
                };
            }(),
            [] {
                auto const pattern = MakeArrowPattern(0, "X", 1, "X", "L");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{AttrAttr(0, "id", 1, "id",
                                                model::gdd::detail::DistanceMetric::kAbsDiff,
                                                model::gdd::detail::CmpOp::kGt, 0.0)});

                return ValidatorCase{
                        .case_name = "HomomorphicMatchAllowsVertexMerging",
                        .temp_file_name = "gdd_homomorphic_vertex_merging.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "X"];
                                1 -> 1 [label = "L"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {},
                        .expected_counterexample_gdd_indices = {0},
                };
            }(),
            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        0 [label = "City"];
                        1 [label = "Country"];
                        2 [label = "Country"];
                        0 -> 1 [label = "capital_of"];
                        0 -> 2 [label = "located_in"];
                    })");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(0, "name", "Impossible")});
                return ValidatorCase{
                        .case_name =
                                "HomomorphicMatchSameTargetForDifferentPatternVertices"
                                "MatchExists",
                        .temp_file_name = "gdd_homomorphic_same_target_match_exists.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "City", name = "Paris"];
                                101 [label = "Country", name = "France"];
                                1 -> 101 [label = "capital_of"];
                                1 -> 101 [label = "located_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {},
                        .expected_counterexample_gdd_indices = {0},
                };
            }(),
            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        0 [label = "City"];
                        1 [label = "Country"];
                        2 [label = "Country"];
                        0 -> 1 [label = "capital_of"];
                        0 -> 2 [label = "located_in"];
                    })");

                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{AttrAttr(1, "name", 2, "name",
                                                model::gdd::detail::DistanceMetric::kEditDistance,
                                                model::gdd::detail::CmpOp::kEq, 0.0)});

                return ValidatorCase{
                        .case_name = "HomomorphicMatchSameTargetForDifferentPatternVertices",
                        .temp_file_name = "gdd_homomorphic_same_target_valid.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "City", name = "Paris"];
                                101 [label = "Country", name = "France"];
                                1 -> 101 [label = "capital_of"];
                                1 -> 101 [label = "located_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {gdd},
                };
            }(),

            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        0 [label = "A"];
                        1 [label = "B"];
                        2 [label = "C"];
                    })");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(0, "name", "Impossible")});
                return ValidatorCase{
                        .case_name = "DisconnectedPatternIsolatedVerticesCanBeMatched",
                        .temp_file_name = "gdd_disconnected_isolated_vertices.dot",
                        .graph_dot = R"(
                            digraph G {
                                10 [label = "A", name = "a"];
                                11 [label = "B", name = "b"];
                                12 [label = "C", name = "c"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {},
                        .expected_counterexample_gdd_indices = {0},
                };
            }(),
            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        0 [label = "A"];
                        1 [label = "B"];
                        0 -> 1 [label = "l"];
                    })");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(1, "name", "Impossible")});
                return ValidatorCase{
                        .case_name = "ConnectedPatternCannotJumpAcrossGraphComponents",
                        .temp_file_name = "gdd_connected_cannot_jump_components.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "A", name = "left"];
                                2 [label = "B", name = "right"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {gdd},
                        .expected_counterexample_gdd_indices = {},
                };
            }(),
            [] {
                auto const pattern = MakePatternPersonCity("lives_in");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(1, "name", "Impossible")});
                return ValidatorCase{
                        .case_name = "ParallelEdgesDifferentLabelsPatternNeedsOne",
                        .temp_file_name = "gdd_parallel_edges_one_needed_label.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "Person", name = "Misha"];
                                2 [label = "City", name = "Amsterdam"];
                                1 -> 2 [label = "works_in"];
                                1 -> 2 [label = "lives_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {},
                        .expected_counterexample_gdd_indices = {0},
                };
            }(),
            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        0 [label = "Person"];
                        1 [label = "City"];
                        0 -> 1 [label = "works_in"];
                        0 -> 1 [label = "lives_in"];
                    })");
                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{EqStrAttrToConst(1, "name", "Impossible")});
                return ValidatorCase{
                        .case_name = "ParallelEdgesDifferentLabelsPatternNeedsBoth",
                        .temp_file_name = "gdd_parallel_edges_both_needed_label.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "Person", name = "Misha"];
                                2 [label = "City", name = "Amsterdam"];
                                1 -> 2 [label = "works_in"];
                                1 -> 2 [label = "lives_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {},
                        .expected_counterexample_gdd_indices = {0},
                };
            }(),
            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        0 [label = "Person"];
                        1 [label = "City"];
                        0 -> 1 [label = "lives_in"];
                        0 -> 1 [label = "works_in"];
                    })");

                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{tests::gdd::utils::AttrConst(
                                      1, "name", std::string("Impossible"),
                                      model::gdd::detail::DistanceMetric::kEditDistance,
                                      model::gdd::detail::CmpOp::kEq, 0.0)});

                return ValidatorCase{
                        .case_name = "PatternHasTwoParallelEdgesDifferentLabelsGraphHasOne",
                        .temp_file_name = "gdd_pattern_parallel_edges_graph_has_one.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "Person", name = "Misha"];
                                2 [label = "City", name = "Amsterdam"];
                                1 -> 2 [label = "lives_in"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {gdd},
                        .expected_counterexample_gdd_indices = {},
                };
            }(),
            [] {
                auto const pattern = ReadGraphFromDot(R"(digraph P {
                        1 [label = "root"];
                        2 [label = "succ"];
                        3 [label = "succ"];

                        1 -> 2 [label = "edge"];
                        1 -> 3 [label = "edge"];
                    })");

                Gdd const gdd(pattern, Gdd::Phi{},
                              Gdd::Phi{AttrAttr(2, "name", 3, "name",
                                                model::gdd::detail::DistanceMetric::kEditDistance,
                                                model::gdd::detail::CmpOp::kGt, 0.0)});

                return ValidatorCase{
                        .case_name = "RootWithTwoSuccChildrenOneComponentSatisfiesAnotherViolates",
                        .temp_file_name = "gdd_root_two_succ_children_one_component_violates.dot",
                        .graph_dot = R"(
                            digraph G {
                                1 [label = "root", name = "v"];
                                2 [label = "succ", name = "l"];
                                3 [label = "succ", name = "r"];
                                1 -> 2 [label = "edge"];
                                1 -> 3 [label = "edge"];

                                4 [label = "root", name = "u"];
                                5 [label = "succ", name = "w"];
                                4 -> 5 [label = "edge"];
                            })",
                        .input_gdds = {gdd},
                        .expected_valid_gdds = {},
                        .expected_counterexample_gdd_indices = {0},
                };
            }(),
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
                        .expected_counterexample_gdd_indices = {0},
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
                        .graph_dot = R"(digraph G {
                                1 [label="Person", name="Misha"];
                                2 [label="Person", name="Bob"];
                                1 -> 2 [label="friend"];
                            })",
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
                        .expected_counterexample_gdd_indices = {0},
                };
            }()};
}

template <class T>
class GddValidatorCasesTest : public ::testing::TestWithParam<ValidatorCase> {
protected:
    struct ValidatorOutput {
        std::vector<Gdd> valid_gdds;
        std::vector<model::GddCounterexample> counterexamples;
        std::vector<std::size_t> matches_count;
    };

    static std::filesystem::path WriteTempDotFile(std::string const& dot,
                                                  std::string const& file_name) {
        auto const path =
                std::filesystem::temp_directory_path() /
                (std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                 file_name);
        std::ofstream out(path);
        out << dot;
        out.close();
        return path;
    }

    static std::unique_ptr<algos::GddValidator> CreateGddValidatorInstance(
            std::filesystem::path const& graph_path, std::vector<Gdd> const& gdds,
            config::ThreadNumType threads) {
        algos::StdParamsMap const option_map = {
                {config::names::kGraphData, graph_path},
                {config::names::kGddData, gdds},
                {config::names::kThreads, threads},
        };
        return algos::CreateAndLoadAlgorithm<T>(option_map);
    }

    static ValidatorOutput CaptureOutput(algos::GddValidator const& validator) {
        return {
                .valid_gdds = validator.GetResult(),
                .counterexamples = validator.GetCounterexamples(),
                .matches_count = validator.GetMatchesCount(),
        };
    }

    static void ExpectCounterexamplesEqual(std::vector<model::GddCounterexample> const& actual,
                                           std::vector<model::GddCounterexample> const& expected) {
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t ce_index = 0; ce_index < actual.size(); ++ce_index) {
            SCOPED_TRACE(ce_index);
            EXPECT_EQ(actual[ce_index].gdd_index, expected[ce_index].gdd_index);
            ASSERT_EQ(actual[ce_index].match.size(), expected[ce_index].match.size());
            for (std::size_t match_index = 0; match_index < actual[ce_index].match.size();
                 ++match_index) {
                auto const& actual_vertex = actual[ce_index].match[match_index];
                auto const& expected_vertex = expected[ce_index].match[match_index];
                EXPECT_EQ(actual_vertex.pattern_vertex_id, expected_vertex.pattern_vertex_id);
                EXPECT_EQ(actual_vertex.pattern_vertex_label, expected_vertex.pattern_vertex_label);
                EXPECT_EQ(actual_vertex.graph_vertex_id, expected_vertex.graph_vertex_id);
                EXPECT_EQ(actual_vertex.graph_vertex_label, expected_vertex.graph_vertex_label);
                EXPECT_EQ(actual_vertex.graph_vertex_attributes,
                          expected_vertex.graph_vertex_attributes);
            }
        }
    }

    static void ExpectCaseResult(ValidatorOutput const& output, ValidatorCase const& tc) {
        EXPECT_EQ(output.valid_gdds, tc.expected_valid_gdds);

        std::vector<std::size_t> actual_counterexample_indices;
        actual_counterexample_indices.reserve(output.counterexamples.size());
        for (auto const& [gdd_index, match] : output.counterexamples) {
            actual_counterexample_indices.push_back(gdd_index);
        }
        EXPECT_EQ(actual_counterexample_indices, tc.expected_counterexample_gdd_indices);
        EXPECT_EQ(output.matches_count.size(), tc.input_gdds.size());
    }

    static void CheckCase(ValidatorCase const& tc) {
        auto const graph_path = WriteTempDotFile(tc.graph_dot, tc.temp_file_name);
        auto const validator =
                CreateGddValidatorInstance(graph_path, tc.input_gdds, config::ThreadNumType{1});
        validator->Execute();
        ValidatorOutput const sequential = CaptureOutput(*validator);
        ExpectCaseResult(sequential, tc);

        validator->SetOption(config::names::kThreads, config::ThreadNumType{4});
        validator->Execute();
        ValidatorOutput const parallel = CaptureOutput(*validator);
        ExpectCaseResult(parallel, tc);
        EXPECT_EQ(parallel.valid_gdds, sequential.valid_gdds);
        ExpectCounterexamplesEqual(parallel.counterexamples, sequential.counterexamples);
        EXPECT_EQ(parallel.matches_count, sequential.matches_count);

        validator->SetOption(config::names::kThreads, config::ThreadNumType{1});
        validator->Execute();
        ValidatorOutput const sequential_again = CaptureOutput(*validator);
        EXPECT_EQ(sequential_again.valid_gdds, sequential.valid_gdds);
        ExpectCounterexamplesEqual(sequential_again.counterexamples, sequential.counterexamples);
        EXPECT_EQ(sequential_again.matches_count, sequential.matches_count);

        if (std::filesystem::exists(graph_path)) {
            std::filesystem::remove(graph_path);
        }
    }
};

TYPED_TEST_SUITE_P(GddValidatorCasesTest);

TYPED_TEST_P(GddValidatorCasesTest, ReturnsExpectedValidGdds) {
    for (ValidatorCase const& tc : GetValidatorCases()) {
        SCOPED_TRACE(tc.case_name);
        TestFixture::CheckCase(tc);
    }
}

TYPED_TEST_P(GddValidatorCasesTest, MatchCountIsPerGddAndStopsAtFirstCounterexample) {
    auto const pattern = tests::gdd::utils::MakeSingleVertexPattern(0, "X");
    Gdd const invalid_gdd(pattern, Gdd::Phi{}, Gdd::Phi{EqStrAttrToConst(0, "name", "Impossible")});
    std::vector const gdds{invalid_gdd, invalid_gdd};
    auto const graph_path = TestFixture::WriteTempDotFile(
            R"(digraph G {
                1 [label = "X", name = "First"];
                2 [label = "X", name = "Second"];
            })",
            "gdd_match_count_stops_at_ce.dot");

    auto const validator =
            TestFixture::CreateGddValidatorInstance(graph_path, gdds, config::ThreadNumType{1});
    validator->Execute();
    EXPECT_EQ(validator->GetMatchesCount(), (std::vector<std::size_t>{1, 1}));

    validator->SetOption(config::names::kThreads, config::ThreadNumType{2});
    validator->Execute();
    EXPECT_EQ(validator->GetMatchesCount(), (std::vector<std::size_t>{1, 1}));

    if (std::filesystem::exists(graph_path)) {
        std::filesystem::remove(graph_path);
    }
}

TYPED_TEST_P(GddValidatorCasesTest, ParallelWorkerExceptionLeavesResultsEmpty) {
    auto const pattern = tests::gdd::utils::MakeSingleVertexPattern(0, "A");
    Gdd const valid_gdd(pattern, Gdd::Phi{}, Gdd::Phi{});
    auto const invalid_constraint = tests::gdd::utils::RelConst(
            0, "knows", model::gdd::detail::ConstValue{std::string{"not-an-id"}});
    Gdd const throwing_gdd(pattern, Gdd::Phi{invalid_constraint}, Gdd::Phi{});
    std::vector const gdds{valid_gdd, throwing_gdd};
    auto const graph_path = TestFixture::WriteTempDotFile(
            R"(digraph G {
                1 [label = "A"];
                2 [label = "B"];
                1 -> 2 [label = "knows"];
            })",
            "gdd_parallel_worker_exception.dot");

    auto const validator =
            TestFixture::CreateGddValidatorInstance(graph_path, gdds, config::ThreadNumType{2});
    EXPECT_THROW(validator->Execute(), std::logic_error);
    EXPECT_TRUE(validator->GetResult().empty());
    EXPECT_TRUE(validator->GetCounterexamples().empty());
    EXPECT_TRUE(validator->GetMatchesCount().empty());

    if (std::filesystem::exists(graph_path)) {
        std::filesystem::remove(graph_path);
    }
}

REGISTER_TYPED_TEST_SUITE_P(GddValidatorCasesTest, ReturnsExpectedValidGdds,
                            MatchCountIsPerGddAndStopsAtFirstCounterexample,
                            ParallelWorkerExceptionLeavesResultsEmpty);

using GddValidatorAlgorithms = ::testing::Types<algos::NaiveGddValidator, algos::WcojGddValidator>;

INSTANTIATE_TYPED_TEST_SUITE_P(GddValidatorCasesTest, GddValidatorCasesTest,
                               GddValidatorAlgorithms);

}  // namespace tests
