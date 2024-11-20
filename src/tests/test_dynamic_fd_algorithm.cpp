#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "config/exceptions.h"
#include "config/indices/type.h"
#include "config/names.h"
#include "csv_config_util.h"
#include "fd/dynfd/dynfd.h"
#include "test_fd_util.h"

namespace tests {
namespace onam = config::names;

struct DynFDAlgorithmParams {
    algos::StdParamsMap params_;
    CSVConfig result_config_;

    explicit DynFDAlgorithmParams(CSVConfig const& insert_config = {},
                                  CSVConfig const& update_config = {},
                                  std::unordered_set<size_t> delete_config = {},
                                  CSVConfig const& csv_config = kTestDynamicFDEmpty,
                                  CSVConfig const& result_config = {})
        : params_({{onam::kCsvConfig, csv_config}}) {
        if (!IsEmpty(insert_config)) {
            params_[onam::kInsertStatements] = MakeInputTable(insert_config);
        }
        if (!IsEmpty(update_config)) {
            params_[onam::kUpdateStatements] = MakeInputTable(update_config);
        }
        if (!delete_config.empty()) {
            params_[onam::kDeleteStatements] = std::move(delete_config);
        }
        result_config_ = result_config;
        params_[config::names::kMaximumLhs] = std::numeric_limits<unsigned int>::max();
    }

    static bool IsEmpty(CSVConfig const& config) {
        static CSVConfig const kEmpty{};
        return config.has_header == kEmpty.has_header && config.path == kEmpty.path &&
               config.separator == kEmpty.separator;
    }
};

// clang-format on

class TestDynFDAlgorithmModify : public ::testing::TestWithParam<DynFDAlgorithmParams> {};

TEST_P(TestDynFDAlgorithmModify, ModifyingTest) {
    auto const& p = GetParam();
    auto const mp = algos::StdParamsMap(p.params_);
    auto const algorithm = algos::CreateAndLoadAlgorithm<algos::dynfd::DynFD>(mp);

    algorithm->Execute();
    auto dynfd_list = algorithm->FdList();

    auto const verification_algo_params = algos::StdParamsMap{
            {config::names::kCsvConfig, p.result_config_},
            {config::names::kMaximumLhs, std::numeric_limits<unsigned int>::max()}};
    auto const verification_algorithm =
            algos::CreateAndLoadAlgorithm<algos::hyfd::HyFD>(verification_algo_params);

    verification_algorithm->Execute();
    auto hyfd_list = verification_algorithm->FdList();

    ASSERT_EQ(FDsToSet(dynfd_list), FDsToSet(hyfd_list));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        DynamicFDAlgorihmTestSuite, TestDynFDAlgorithmModify,
        ::testing::Values(
            DynFDAlgorithmParams(kTestDynamicFDInsert, {}, {}, kTestDynamicFDEmpty, kTestDynamicFDInsert),
            DynFDAlgorithmParams(kTestDynamicFDInsert, {}, {}, kTestDynamicFDInit, kTestDynamicFDAfterInsert),
            DynFDAlgorithmParams({}, {}, {1, 6, 3}, kTestDynamicFDInit, kTestDynamicFDAfterDelete),
            DynFDAlgorithmParams(kTestDynamicFDInsert, kTestDynamicFDUpdate, {}, kTestDynamicFDInit, kTestDynamicFDAfterInsertAndUpdate),
            DynFDAlgorithmParams({}, kTestDynamicFDUpdate, {1, 6, 3}, kTestDynamicFDInit, kTestDynamicFDAfterUpdateAndDelete),
            DynFDAlgorithmParams(kTestDynamicFDInsert, kTestDynamicFDUpdate, {1, 6, 3}, kTestDynamicFDInit, kTestDynamicFDAfterAll)
            ));

// clang-format onÏ

}  // namespace tests
