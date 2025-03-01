#include <algorithm>  // for sort, transform
#include <exception>  // for exception
#include <iostream>   // for char_traits, basic_ostream
#include <iterator>   // for back_inserter
#include <memory>     // for unique_ptr
#include <string>     // for hash
#include <utility>    // for pair
#include <vector>     // for vector

#include <gmock/gmock.h>  // for ContainerEq, ASSERT_THAT
#include <gtest/gtest.h>  // for Message, Values, Paramete...

#include "algorithms/algo_factory.h"        // for CreateAndLoadAlgorithm
#include "algorithms/fd/pyro/pyro.h"        // for Pyro
#include "all_csv_configs.h"                // for kAbalone, kAdult, kCIPubl...
#include "config/error/type.h"              // for ErrorType
#include "config/names.h"                   // for kCsvConfig, kError, kSeed
#include "fd/fd_algorithm.h"                // for FDAlgorithm
#include "fd/pli_based_fd_algorithm.h"      // for PliBasedFDAlgorithm
#include "fd/pyrocommon/core/parameters.h"  // for Parameters
#include "table/column.h"                   // for Column

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
