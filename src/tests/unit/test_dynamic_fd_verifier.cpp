#include <algorithm>
#include <memory>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "config/exceptions.h"
#include "config/indices/type.h"
#include "config/names.h"
#include "csv_config_util.h"
#include "fd/fd_verifier/dynamic_fd_verifier.h"
#include "fd/fd_verifier/dynamic_stats_calculator.h"
#include "model/types/builtin.h"

namespace {
using namespace algos::fd_verifier;

void TestSorting(std::unique_ptr<DynamicFDVerifier> verifier) {
    auto const& highlights = verifier->GetHighlights();
    if (highlights.size() < 2) {
        return;
    }
    verifier->SortHighlightsByProportionDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByProportionDescending()));
    verifier->SortHighlightsByProportionAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByProportionAscending()));
    verifier->SortHighlightsBySizeDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsBySizeDescending()));
    verifier->SortHighlightsBySizeAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsBySizeAscending()));
    verifier->SortHighlightsByNumDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByNumDescending()));
    verifier->SortHighlightsByNumAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByNumAscending()));
}
}  // namespace

namespace tests {
namespace onam = config::names;

struct DynFDVerifyingParams {
    algos::StdParamsMap params;
    long double const error = 0.;
    size_t const num_error_clusters = 0;
    size_t const num_error_rows = 0;

    DynFDVerifyingParams(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
                         size_t const num_error_clusters = 0, size_t const num_error_rows = 0,
                         long double const error = 0., CSVConfig const& insert_config = {},
                         CSVConfig const& update_config = {},
                         std::unordered_set<size_t> delete_config = {},
                         CSVConfig const& csv_config = kTestDynamicFDInit)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)}}),
          error(error),
          num_error_clusters(num_error_clusters),
          num_error_rows(num_error_rows) {
        if (!IsEmpty(insert_config)) {
            params[onam::kInsertStatements] = MakeInputTable(insert_config);
        }
        if (!IsEmpty(update_config)) {
            params[onam::kUpdateStatements] = MakeInputTable(update_config);
        }
        if (!delete_config.empty()) {
            params[onam::kDeleteStatements] = std::move(delete_config);
        }
    }

    bool IsEmpty(CSVConfig const& config) {
        static CSVConfig const kEmpty{};
        return config.has_header == kEmpty.has_header && config.path == kEmpty.path &&
               config.separator == kEmpty.separator;
    }
};

class TestDynFDVerifyingInit : public ::testing::TestWithParam<DynFDVerifyingParams> {};

TEST_P(TestDynFDVerifyingInit, InitializationTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::fd_verifier::DynamicFDVerifier>(mp);
    EXPECT_EQ(verifier->FDHolds(), p.num_error_clusters == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRows(), p.num_error_rows);
    EXPECT_EQ(verifier->GetNumErrorClusters(), p.num_error_clusters);

    TestSorting(std::move(verifier));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        DynamicFDVerifierTestSuite, TestDynFDVerifyingInit,
        ::testing::Values(
            DynFDVerifyingParams({0}, {1}, 0, 0, 0., {}, {}, {}, kTestDynamicFDEmpty),
            DynFDVerifyingParams({0, 1, 2, 3, 4}, {5}, 0, 0, 0.),
            DynFDVerifyingParams({1, 2}, {0, 3}, 1, 2, 2.L/132),
            DynFDVerifyingParams({2, 4}, {0, 1, 3, 5}, 0, 0, 0.),
            DynFDVerifyingParams({1}, {2, 3}, 4, 12, 18.L/132),
            DynFDVerifyingParams({1, 4}, {2, 3, 5}, 3, 8, 10.L/132),
            DynFDVerifyingParams({0, 1}, {1, 4}, 2, 6, 8.L/132)
            ));

// clang-format on

class TestDynFDVerifyingModify : public ::testing::TestWithParam<DynFDVerifyingParams> {};

TEST_P(TestDynFDVerifyingModify, ModifyingTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::fd_verifier::DynamicFDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->FDHolds(), p.num_error_clusters == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRows(), p.num_error_rows);
    EXPECT_EQ(verifier->GetNumErrorClusters(), p.num_error_clusters);

    TestSorting(std::move(verifier));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        DynamicFDVerifierTestSuite, TestDynFDVerifyingModify,
        ::testing::Values(
            DynFDVerifyingParams({0}, {1}, 0, 0, 0., kTestDynamicFDInsert, {}, {}, kTestDynamicFDEmpty),
            DynFDVerifyingParams({0, 1, 2, 3, 4}, {5}, 1, 2, 1.L/105, kTestDynamicFDInsert),
            DynFDVerifyingParams({4}, {3}, 0, 0, 0., {}, kTestDynamicFDUpdate),
            DynFDVerifyingParams({1, 2}, {0, 3}, 0, 0, 0., {}, {}, {1, 6, 3}),
            DynFDVerifyingParams({2, 4}, {0, 1, 3, 5}, 2, 4, 2.L/105, kTestDynamicFDInsert, kTestDynamicFDUpdate),
            DynFDVerifyingParams({1}, {2, 3}, 5, 12, 7.L/66, kTestDynamicFDInsert, {}, {1, 6, 3}),
            DynFDVerifyingParams({1, 4}, {2, 3, 5}, 2, 5, 1.L/12, {}, kTestDynamicFDUpdate, {1, 6, 3}),
            DynFDVerifyingParams({0, 1}, {1, 4}, 2, 5, 1.L/22, kTestDynamicFDInsert, kTestDynamicFDUpdate, {1, 6, 3})
            ));

// clang-format on

class TestDynFDVerifyingExceptions : public ::testing::TestWithParam<DynFDVerifyingParams> {};

void CreateLoadExeute(algos::StdParamsMap const& params) {
    auto verifier = algos::CreateAndLoadAlgorithm<algos::fd_verifier::DynamicFDVerifier>(params);
    verifier->Execute();
}

TEST_P(TestDynFDVerifyingExceptions, ExceptionsTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    ASSERT_THROW(CreateLoadExeute(mp), config::ConfigurationError);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        DynamicFDVerifierTestSuite, TestDynFDVerifyingExceptions,
        ::testing::Values(
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., kTestDynamicFDInsertBad1),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., kTestDynamicFDInsertBad2),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., {}, kTestDynamicFDUpdateBad1),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., {}, kTestDynamicFDUpdateBad2),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., {}, kTestDynamicFDUpdateBad3),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., {}, kTestDynamicFDUpdateBad4),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., {}, {}, {100000}),
            DynFDVerifyingParams({1}, {2}, 3, 4, 5., {}, kTestDynamicFDUpdate, {4})
            ));

// clang-format on

}  // namespace tests
