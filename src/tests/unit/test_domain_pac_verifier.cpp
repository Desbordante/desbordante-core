#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/pac/model/default_domains/ball.h"
#include "core/algorithms/pac/model/default_domains/domain_type.h"
#include "core/algorithms/pac/model/default_domains/parallelepiped.h"
#include "core/algorithms/pac/model/default_domains/untyped_domain.h"
#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_cli_adapter.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/model/types/builtin.h"
#include "core/model/types/mixed_type.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
using namespace config::names;
using namespace pac::model;

// Threshold for algo results (epsilon and delta)
constexpr static auto kThreshold = 1e-3;

struct DomainPACVerifyingParams {
    algos::StdParamsMap params;
    double exp_epsilon;
    double exp_delta;

    DomainPACVerifyingParams(CSVConfig const& csv_config, config::IndicesType&& col_indices,
                             std::shared_ptr<pac::model::IDomain>&& domain, double expected_epsilon,
                             double expected_delta, double min_delta = -1, double min_eps = -1,
                             double max_eps = -1, unsigned long delta_steps = 0,
                             bool dist_from_null_is_infinity = false,
                             double diagonal_threshold = 1e-5)
        : params({{kCsvConfig, csv_config},
                  {kColumnIndices, std::move(col_indices)},
                  {kDomain, std::move(domain)},
                  {kMinDelta, min_delta},
                  {kMinEpsilon, min_eps},
                  {kMaxEpsilon, max_eps},
                  {kDeltaSteps, delta_steps},
                  {kDistFromNullIsInfinity, dist_from_null_is_infinity},
                  {kDiagonalThreshold, diagonal_threshold}}),
          exp_epsilon(expected_epsilon),
          exp_delta(expected_delta) {}
};

class TestDomainPACVerifier : public ::testing::TestWithParam<DomainPACVerifyingParams> {};

TEST_P(TestDomainPACVerifier, DefaultTest) {
    auto const& p = GetParam();
    auto params_map = p.params;
    auto verifier =
            algos::CreateAndLoadAlgorithm<algos::pac_verifier::DomainPACVerifier>(params_map);
    verifier->Execute();

    auto const& pac = verifier->GetPAC();
    EXPECT_NEAR(pac.GetEpsilon(), p.exp_epsilon, kThreshold);
    EXPECT_NEAR(pac.GetDelta(), p.exp_delta, kThreshold);
}

struct DomainPACVerifyingPythonParams {
    algos::StdParamsMap params;
    double exp_epsilon;
    double exp_delta;

    DomainPACVerifyingPythonParams(
            CSVConfig const& csv_config, config::IndicesType&& col_indices, double expected_epsilon,
            double expected_delta, DomainType domain_type,
            std::vector<double>&& leveling_coeffs = {}, std::vector<std::string>&& center = {},
            double radius = 0, std::vector<std::string>&& first = {},
            std::vector<std::string>&& last = {},
            StringDistFromDomain&& string_dist_from_domain = nullptr,
            std::string&& domain_name = "", double min_delta = -1, unsigned long delta_steps = 0,
            bool dist_from_domain_is_infty = false, double diagonal_threshold = 1e-5)
        : params({{kCsvConfig, csv_config},
                  {kColumnIndices, std::move(col_indices)},
                  {kDomainType, domain_type},
                  {kMinDelta, min_delta},
                  {kDeltaSteps, delta_steps},
                  {kDistFromNullIsInfinity, dist_from_domain_is_infty},
                  {kDiagonalThreshold, diagonal_threshold}}),
          exp_epsilon(expected_epsilon),
          exp_delta(expected_delta) {
        switch (domain_type) {
            case pac::model::DomainType::ball:
                params[kLevelingCoeffs] = std::move(leveling_coeffs);
                params[kCenter] = std::move(center);
                params[kRadius] = radius;
                break;
            case pac::model::DomainType::parallelepiped:
                params[kLevelingCoeffs] = std::move(leveling_coeffs);
                params[kFirst] = std::move(first);
                params[kLast] = std::move(last);
                break;
            case pac::model::DomainType::custom_domain:
                params[kStringDistFromDomain] = std::move(string_dist_from_domain);
                params[kDomainName] = std::move(domain_name);
                break;
        }
    }
};

class TestDomainPACVerifierPython
    : public ::testing::TestWithParam<DomainPACVerifyingPythonParams> {};

TEST_P(TestDomainPACVerifierPython, DefaultTest) {
    auto const& p = GetParam();
    auto params_map = p.params;
    auto verifier = algos::CreateAndLoadAlgorithm<algos::pac_verifier::DomainPACVerifierCLIAdapter>(
            params_map);
    verifier->Execute();

    auto const& pac = verifier->GetPAC();
    EXPECT_NEAR(pac.GetEpsilon(), p.exp_epsilon, kThreshold);
    EXPECT_NEAR(pac.GetDelta(), p.exp_delta, kThreshold);
}

/// @brief Is single-column mixed-typed value a null
inline bool IsNull(Tuple const& value) {
    auto const* real_value = model::MixedType::RetrieveValue(value[0]);
    auto const type_id = model::MixedType::RetrieveTypeId(value[0]);
    return type_id == +model::TypeId::kNull || real_value == nullptr;
}

/// @brief x in D iff x is not null.
/// Works on a single column of mixed type.
class NotNullDomain final : public IDomain {
private:
    model::MixedType mixed_type_;

public:
    NotNullDomain() : mixed_type_(true) {
        tuple_type_ = std::make_shared<TupleType>(std::vector<model::Type const*>{&mixed_type_});
    }

    virtual double DistFromDomain(Tuple const& value) const override {
        return IsNull(value) ? 1 : 0;
    }

    virtual std::string ToString() const override {
        return "{x : x is not null}";
    }
};

using Strings = std::vector<std::string>;

DomainPACVerifyingParams CustomMetricBallsIntervalsParams(double min_eps, double max_eps,
                                                          double expected_eps,
                                                          double expected_delta,
                                                          double min_delta = -1,
                                                          unsigned long delta_steps = 0) {
    return {kCustomMetricBalls,
            {0, 1},
            std::make_shared<Ball>(Strings{"0", "0"}, 5),
            expected_eps,
            expected_delta,
            min_delta,
            min_eps,
            max_eps,
            delta_steps};
}

INSTANTIATE_TEST_SUITE_P(
        DomainPACVerifierTests, TestDomainPACVerifier,
        ::testing::Values(
                // -- "Refinement" --
                // (finding optimal eps, delta)
                // A simple test on 1D-array
                // #0
                DomainPACVerifyingParams(kSimpleTypos, {1},
                                         std::make_shared<Parallelepiped>("0", "5"), 0, 0.9, 0.8),
                // A simple test on 1D-array, but values don't fall into domain
                DomainPACVerifyingParams(kSimpleTypos, {1},
                                         std::make_shared<Parallelepiped>("5", "7"), 4, 1),
                // A ball in R^2 with center (0, 0) and radius 5
                DomainPACVerifyingParams(kCustomMetricBalls, {0, 1},
                                         std::make_shared<Ball>(Strings{"0", "0"}, 5), 6.217, 0.999,
                                         0.7),
                // A square-shaped domain in R^2 with corners (-5, -5) and (5, 5)
                DomainPACVerifyingParams(kCustomMetricBalls, {2, 3},
                                         std::make_shared<Parallelepiped>(Strings{"-5", "-5"},
                                                                          Strings{"5", "5"}),
                                         4.379, 0.999, 0.7),
                // Check not-metrizable type:
                //  a. only not-null values
                DomainPACVerifyingParams(kMixedWithNulls, {0}, std::make_shared<NotNullDomain>(), 0,
                                         0.8, 0.7),
                //  b. all values
                DomainPACVerifyingParams(kMixedWithNulls, {0}, std::make_shared<NotNullDomain>(), 1,
                                         1, 0.9),
                // Special cases:
                //	a. +\infty
                DomainPACVerifyingParams(kSimpleTypos, {1},
                                         std::make_shared<Parallelepiped>("11", "11"), 8, 0.4, 0.3),
                // 	b. -\infty
                DomainPACVerifyingParams(kSimpleTypos, {1},
                                         std::make_shared<Parallelepiped>("0", "0"), 4, 0.9, 0.3),
                // Test leveling coefficients
                DomainPACVerifyingParams(kTestDC1, {1, 2},
                                         std::make_shared<Ball>(Strings{"3500", "0.2"}, 1,
                                                                std::vector<double>{1e-3, 10}),
                                         0.803, 0.8, 0.7),
                // Test Untyped Domain
                DomainPACVerifyingParams(kSimpleTypos, {1},
                                         std::make_shared<UntypedDomain>(
                                                 [](Strings const& value) {
                                                     auto val = std::stoi(value.front());
                                                     if (val < 0) {
                                                         return -val;
                                                     }
                                                     if (val > 5) {
                                                         return val - 5;
                                                     }
                                                     return 0;
                                                 },
                                                 "[\"0\", \"5\"]"),
                                         0, 0.9, 0.8),
                // -- Paramethrized "refinement" --
                // (finding optimal eps, delta on some segment of ECDF)
                // Example from Comparison slides
                // (https://github.com/p-senichenkov/Domain-PAC-validation-comparison/blob/main/slides/slides.pdf)
                // #10
                CustomMetricBallsIntervalsParams(0, 1, 0.631, 0.625),
                CustomMetricBallsIntervalsParams(1.2, 1.3, 1.225, 0.73),
                CustomMetricBallsIntervalsParams(2, 5, 4.709, 0.991),
				// (min_eps, ??) is considered a point too, and here it wins
                DomainPACVerifyingParams(kSimpleTypos, {1},
                                         std::make_shared<Parallelepiped>("5", "7"), 1, 0.5, 0.5,
                                         1),
                // Min_delta is greater than delta for max_eps => should return (??, min_delta)
                CustomMetricBallsIntervalsParams(0, 1, 2.60487, 0.9, 0.9),
                // -- Validation --
                // (finding delta for the given eps)
                // #15
                CustomMetricBallsIntervalsParams(1.5, 1.5, 1.5, 0.774),
                // Min delta is greater than actual delta => should return actual delta
                CustomMetricBallsIntervalsParams(1.5, 1.5, 1.5, 0.774, 0.9, 1000)));

// Same, but Python versions
INSTANTIATE_TEST_SUITE_P(
        DomainPACVeriferPythonTests, TestDomainPACVerifierPython,
        ::testing::Values(
                // 1D array
                DomainPACVerifyingPythonParams(kSimpleTypos, {1}, 0, 0.9,
                                               DomainType::parallelepiped, {}, {}, 0, {"0"}, {"5"},
                                               nullptr, "", 0.8),
                // 2D ball
                DomainPACVerifyingPythonParams(kCustomMetricBalls, {0, 1}, 6.217, 0.999,
                                               DomainType::ball, {}, {"0", "0"}, 5, {}, {}, nullptr,
                                               "", 0.7),
                // 2D rectangle
                DomainPACVerifyingPythonParams(kCustomMetricBalls, {2, 3}, 4.379, 0.999,
                                               DomainType::parallelepiped, {}, {}, 0, {"-5", "-5"},
                                               {"5", "5"}, nullptr, "", 0.7)));

using Epsilons = std::pair<double, double>;
using HighlightValues = std::vector<std::string>;

struct DomainPACHighlightParams {
    algos::StdParamsMap params;
    std::map<Epsilons, HighlightValues> expected_highlights;

    DomainPACHighlightParams(CSVConfig const& csv_config, config::IndicesType&& col_indices,
                             std::shared_ptr<pac::model::IDomain>&& domain,
                             std::map<Epsilons, HighlightValues>&& expected_highlights,
                             bool dist_from_null_is_infinity = false)
        : params({{kCsvConfig, csv_config},
                  {kColumnIndices, std::move(col_indices)},
                  {kDomain, std::move(domain)},
                  {kDistFromNullIsInfinity, dist_from_null_is_infinity}}),
          expected_highlights(std::move(expected_highlights)) {}
};

class TestDomainPACHighlight : public ::testing::TestWithParam<DomainPACHighlightParams> {};

TEST_P(TestDomainPACHighlight, DefaultTest) {
    auto const& p = GetParam();
    auto algo = algos::CreateAndLoadAlgorithm<algos::pac_verifier::DomainPACVerifier>(p.params);
    algo->Execute();

    for (auto const& [epsilons, expected_values] : p.expected_highlights) {
        auto actual_highlight = algo->GetHighlights(epsilons.first, epsilons.second);
        EXPECT_THAT(actual_highlight.GetStringData(),
                    ::testing::UnorderedElementsAreArray(expected_values));
    }
}

INSTANTIATE_TEST_SUITE_P(
        DomainPACVerifierHighlightsTests, TestDomainPACHighlight,
        ::testing::Values(
                // Some values fall into domain, highlighed values on both sides of domain
                DomainPACHighlightParams(kTest1, {0}, std::make_shared<Parallelepiped>("4", "6"),
                                         {{{0.1, 1.1}, {"3", "7"}},
                                          {{1.1, 2.1}, {"2", "2", "2", "8"}},
                                          {{2.1, 3.1}, {"1", "1", "1", "1", "9"}}},
                                         true),
                // No values fall into domain, highlighted values on both sides of domain
                DomainPACHighlightParams(kSimpleTypos, {2},
                                         std::make_shared<Ball>(Strings{"17"}, 4),
                                         {{
                                                  {0.1, 1.1},
                                                  {"22", "22"},
                                          },
                                          {{1.1, 2.1}, {"11", "11", "11", "11"}},
                                          {{2.1, 5.1}, {"10"}},
                                          {{5.1, 25.1}, {"33", "34", "44"}}}),
                // Custom domain
                DomainPACHighlightParams(kSimpleTypos, {1},
                                         std::make_shared<UntypedDomain>(
                                                 [](Strings const& value) {
                                                     auto val = std::stoi(value.front());
                                                     if (val < 3) {
                                                         return 3 - val;
                                                     }
                                                     if (val > 8) {
                                                         return val - 8;
                                                     }
                                                     return 0;
                                                 },
                                                 "[\"3\", \"8\"]"),
                                         {{{0.1, 1.1}, {"2", "2"}},
                                          {{1.1, 2.1}, {"1", "1", "1", "1", "10"}}})));
}  // namespace tests
