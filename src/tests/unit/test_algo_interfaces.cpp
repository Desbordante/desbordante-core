#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/pyro/pyro.h"
#include "all_csv_configs.h"
#include "config/error/type.h"
#include "config/names.h"
#include "fd/fd_algorithm.h"
#include "fd/pli_based_fd_algorithm.h"
#include "fd/pyrocommon/core/parameters.h"
#include "table/column.h"

struct CSVConfig;

namespace tests {

struct KeysTestParams {
    std::vector<unsigned int> const expected;
    CSVConfig const& csv_config;

    KeysTestParams(std::vector<unsigned int>&& expected, CSVConfig const& csv_config)
        : expected(expected), csv_config(csv_config) {}
};

class KeysTest : public ::testing::TestWithParam<KeysTestParams> {};

template <typename AlgoInterface>
inline static void GetKeysTestImpl(KeysTestParams const& p) {
    using namespace config::names;
    using ::testing::ContainerEq;

    std::vector<unsigned int> actual;
    algos::StdParamsMap params_map{{kCsvConfig, p.csv_config},
                                   {kSeed, decltype(algos::pyro::Parameters::seed){0}},
                                   {kError, config::ErrorType{0.0}}};
    auto pyro_ptr = algos::CreateAndLoadAlgorithm<algos::Pyro>(params_map);
    auto& pyro = *pyro_ptr;

    pyro.Execute();

    try {
        std::vector<Column const*> const keys = pyro.AlgoInterface::GetKeys();
        actual.reserve(keys.size());
        std::transform(keys.cbegin(), keys.cend(), std::back_inserter(actual),
                       [](Column const* col) { return col->GetIndex(); });
    } catch (std::exception const& e) {
        std::cout << "Exception raised in test: " << e.what() << std::endl;
        FAIL();
    }

    std::sort(actual.begin(), actual.end());

    ASSERT_THAT(actual, ContainerEq(p.expected));
}

TEST_P(KeysTest, FDAlgorithmTest) {
    GetKeysTestImpl<algos::FDAlgorithm>(GetParam());
}

TEST_P(KeysTest, PliBasedAlgorithmTest) {
    GetKeysTestImpl<algos::PliBasedFDAlgorithm>(GetParam());
}

INSTANTIATE_TEST_SUITE_P(, KeysTest,
                         ::testing::Values(KeysTestParams({0, 1, 2}, kWdcAge),
                                           KeysTestParams({0, 1, 2, 3, 4}, kWdcGame),
                                           KeysTestParams({0, 2}, kWdcAppearances),
                                           KeysTestParams({3, 4, 5}, kWdcAstronomical),
                                           KeysTestParams({0, 2}, kCIPublicHighway700),
                                           KeysTestParams({}, kAbalone),
                                           KeysTestParams({}, kAdult)));
}  // namespace tests
