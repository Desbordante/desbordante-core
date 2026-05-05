#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cind/cind_algorithm.h"
#include "core/algorithms/cind/types.h"
#include "core/config/error/type.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
namespace {
struct CINDNumberParams final {
    algos::StdParamsMap const params;
    size_t const expected_cinds;

    CINDNumberParams(config::ErrorType const aind_error_, size_t expected_cinds_,
                     CSVConfigs const& csv_configs = {kTestCINDDe, kTestCINDEn})
        : params({{config::names::kCsvConfigs, csv_configs}, {config::names::kError, aind_error_}}),
          expected_cinds(expected_cinds_) {}
};

struct CINDConditionsParams final {
    algos::StdParamsMap const params;
    size_t const expected_conditions;
    config::ErrorType const validity_threshold;
    config::ErrorType const completeness_threshold;

    CINDConditionsParams(algos::cind::AlgoType const algo_type,
                         algos::cind::CondType const cond_type,
                         config::ErrorType const validity_threshold_,
                         config::ErrorType const completeness_threshold_,
                         size_t expected_conditions_,
                         CSVConfigs const& csv_configs = {kTestCINDDe, kTestCINDEn})
        : params({{config::names::kCsvConfigs, csv_configs},
                  {config::names::kAlgoType, algo_type},
                  {config::names::kConditionType, cond_type},
                  {config::names::kError, 0.5},
                  {config::names::kValidity, validity_threshold_},
                  {config::names::kCompleteness, completeness_threshold_}}),
          expected_conditions(expected_conditions_),
          validity_threshold(validity_threshold_),
          completeness_threshold(completeness_threshold_) {}
};
}  // namespace

class TestCINDNumber : public ::testing::TestWithParam<CINDNumberParams> {};

TEST_P(TestCINDNumber, SimpleTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto cind_algo = algos::CreateAndLoadAlgorithm<algos::cind::CindAlgorithm>(mp);
    cind_algo->Execute();
    ASSERT_EQ(cind_algo->CINDList().size(), p.expected_cinds);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    CINDTestSuite, TestCINDNumber, ::testing::Values(
        CINDNumberParams(0, 3),
        CINDNumberParams(0.3, 4),
        CINDNumberParams(0.4, 5),
        CINDNumberParams(0.5, 8)
    ));
// clang-format on

class TestCINDConditions : public ::testing::TestWithParam<CINDConditionsParams> {};

TEST_P(TestCINDConditions, SimpleTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto cind_algo = algos::CreateAndLoadAlgorithm<algos::cind::CindAlgorithm>(mp);
    cind_algo->Execute();
    ASSERT_EQ(cind_algo->CINDList().front().ConditionsNumber(), p.expected_conditions);
    for (auto const& cind : cind_algo->CINDList()) {
        for (auto const& condition : cind.conditions) {
            ASSERT_GE(condition.validity, p.validity_threshold);
            ASSERT_GE(condition.completeness, p.completeness_threshold);
        }
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    CINDTestSuite, TestCINDConditions, ::testing::Values(
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::row, 0.0, 0.01, 61),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::row, 0.0, 0.15, 23),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::row, 0.0, 0.56, 3),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::row, 1.0, 0.01, 56),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::row, 1.0, 0.15, 18),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::row, 1.0, 0.56, 2),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::row, 0.0, 0.01, 61),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::row, 0.0, 0.15, 23),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::row, 0.0, 0.56, 3),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::row, 1.0, 0.01, 56),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::row, 1.0, 0.15, 18),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::row, 1.0, 0.56, 2),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::group, 0.1, 0.4, 61),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::group, 0.1, 0.6, 1),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::group, 0.75, 0.4, 56),
        CINDConditionsParams(algos::cind::AlgoType::cinderella, algos::cind::CondType::group, 0.75, 0.6, 0),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::group, 0.1, 0.4, 61),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::group, 0.1, 0.6, 1),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::group, 0.75, 0.4, 56),
        CINDConditionsParams(algos::cind::AlgoType::pli_cind, algos::cind::CondType::group, 0.75, 0.6, 0)
    ));
// clang-format on

}  // namespace tests
