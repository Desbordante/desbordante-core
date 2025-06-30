#include <algorithm>   // for sort
#include <filesystem>  // for operator==, path
#include <iostream>    // for char_traits, basi...
#include <list>        // for list, _List_iterator
#include <memory>      // for unique_ptr, make_...
#include <set>         // for set, operator==
#include <stddef.h>    // for size_t
#include <stdexcept>   // for runtime_error
#include <string>      // for hash, string, ope...
#include <utility>     // for pair, make_pair
#include <vector>      // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset
#include <gmock/gmock.h>                            // for ContainerEq, Eq
#include <gtest/gtest.h>                            // for AssertionResult

#include "algorithms/algo_factory.h"        // for CreateAndLoadAlgo...
#include "algorithms/fd/fd_mine/fd_mine.h"  // for FdMine
#include "algorithms/fd/pyro/pyro.h"        // for Pyro
#include "all_csv_configs.h"                // for kBreastCancer
#include "config/error/type.h"              // for ErrorType
#include "config/names.h"                   // for kCsvConfig, kError
#include "csv_config_util.h"                // for MakeInputTable
#include "csv_parser/csv_parser.h"          // for CSVConfig
#include "fd/fd.h"                          // for FD
#include "fd/fd_algorithm.h"                // for FDAlgorithm
#include "fd/pyrocommon/core/parameters.h"  // for Parameters
#include "fd/raw_fd.h"                      // for RawFD
#include "table/column.h"                   // for Column
#include "table/vertical.h"                 // for Vertical
#include "test_fd_util.h"                   // for AlgorithmTest

namespace tests {
using ::testing::ContainerEq, ::testing::Eq;

using algos::FDAlgorithm, algos::FdMine, algos::StdParamsMap;

namespace onam = config::names;

StdParamsMap FdMineGetParamMap(CSVConfig const& csv_config) {
    return {{config::names::kTable, MakeInputTable(csv_config)}};
}

std::unique_ptr<FDAlgorithm> ConfToLoadFdMine(CSVConfig const& csv_config) {
    std::unique_ptr<FDAlgorithm> algorithm = std::make_unique<FdMine>();
    algos::ConfigureFromMap(*algorithm, FdMineGetParamMap(csv_config));
    return algorithm;
}

std::unique_ptr<FDAlgorithm> CreateFdMineAlgorithmInstance(CSVConfig const& csv_config) {
    return algos::CreateAndLoadAlgorithm<FdMine>(FdMineGetParamMap(csv_config));
}

using FDMineAlgorithmTest = tests::AlgorithmTest<FdMine>;

std::vector<unsigned int> FdMineBitsetToIndexVector(boost::dynamic_bitset<> const& bitset) {
    std::vector<unsigned int> res;
    for (size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
         index = bitset.find_next(index)) {
        res.push_back(index);
    }
    return res;
}

testing::AssertionResult FdMineCheckFdListEquality(
        std::set<std::pair<std::vector<unsigned int>, unsigned int>> actual,
        std::list<FD> const& expected) {
    for (auto& fd : expected) {
        std::vector<unsigned int> lhs_indices =
                FdMineBitsetToIndexVector(fd.GetLhs().GetColumnIndices());
        std::sort(lhs_indices.begin(), lhs_indices.end());

        if (auto it = actual.find(std::make_pair(lhs_indices, fd.GetRhs().GetIndex()));
            it == actual.end()) {
            return testing::AssertionFailure()
                   << "discovered a false FD: " << fd.GetLhs().ToIndicesString() << "->"
                   << fd.GetRhs().ToIndicesString();
        } else {
            actual.erase(it);
        }
    }
    return actual.empty() ? testing::AssertionSuccess()
                          : testing::AssertionFailure() << "some FDs remain undiscovered";
}

std::set<std::pair<std::vector<unsigned int>, unsigned int>> FdMineFDsToSet(
        std::list<FD> const& fds) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> set;
    for (auto const& fd : fds) {
        auto const& raw_fd = fd.ToRawFD();
        set.emplace(FdMineBitsetToIndexVector(raw_fd.lhs_), raw_fd.rhs_);
    }
    return set;
}

TEST(AlgorithmSyntheticTest, FD_Mine_ThrowsOnEmpty) {
    auto algorithm = ConfToLoadFdMine(tests::kTestEmpty);
    ASSERT_THROW(algorithm->LoadData(), std::runtime_error);
}

TEST(AlgorithmSyntheticTest, FD_Mine_ReturnsEmptyOnSingleNonKey) {
    auto algorithm = CreateFdMineAlgorithmInstance(tests::kTestSingleColumn);
    algorithm->Execute();
    ASSERT_TRUE(algorithm->FdList().empty());
}

TEST(AlgorithmSyntheticTest, FD_Mine_WorksOnLongDataset) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{{{2}, 1}};

    auto algorithm = CreateFdMineAlgorithmInstance(tests::kTestLong);
    algorithm->Execute();
    ASSERT_TRUE(FdMineCheckFdListEquality(true_fd_collection, algorithm->FdList()));
}

std::string GetJsonFDs(std::list<FD>& fd_collection) {
    std::string result = "{\"fds\": [";
    std::list<std::string> discovered_fd_strings;
    for (auto& fd : fd_collection) {
        discovered_fd_strings.push_back(fd.ToJSONString());
    }
    discovered_fd_strings.sort();
    for (auto const& fd : discovered_fd_strings) {
        result += fd + ",";
    }
    if (result.back() == ',') {
        result.erase(result.size() - 1);
    }

    result += "]}";
    return result;
}

void MinimizeFDs(std::list<FD>& fd_collection) {
    std::list<FD>::iterator it1 = fd_collection.begin();
    while (it1 != fd_collection.end()) {
        std::list<FD>::iterator it2 = fd_collection.begin();
        while (it2 != fd_collection.end()) {
            if (it1 == it2) {
                it2++;
                continue;
            }
            if (it1->GetRhs().GetIndex() == it2->GetRhs().GetIndex()) {
                auto lhs1 = it1->GetLhs();
                auto lhs2 = it2->GetLhs();
                if (lhs2.Contains(lhs1)) {
                    it2 = fd_collection.erase(it2);
                    continue;
                }
            }
            it2++;
        }
        it1++;
    }
}

TEST_F(FDMineAlgorithmTest, FD_Mine_ReturnsSameAsPyro) {
    using namespace config::names;

    try {
        for (auto const& [config, hash] : FDMineAlgorithmTest::kLightDatasets) {
            // TODO: change this hotfix
            if (config.path == tests::kBreastCancer.path) {
                continue;
            }
            auto algorithm = CreateFdMineAlgorithmInstance(config);

            StdParamsMap params_map{{kCsvConfig, config},
                                    {kSeed, decltype(algos::pyro::Parameters::seed){0}},
                                    {kError, config::ErrorType{0.0}}};
            auto pyro_ptr = algos::CreateAndLoadAlgorithm<algos::Pyro>(params_map);
            auto& pyro = *pyro_ptr;

            algorithm->Execute();
            std::list<FD> fds = algorithm->FdList();
            pyro.Execute();

            for (auto& fd : pyro.FdList()) {
                if (fd.GetLhs().GetArity() == 0) {
                    std::list<FD>::iterator it = fds.begin();
                    while (it != fds.end()) {
                        if (it->GetRhs().GetIndex() == fd.GetRhs().GetIndex()) {
                            it = fds.erase(it);
                            continue;
                        }
                        it++;
                    }
                    fds.push_back(fd);
                }
            }

            MinimizeFDs(fds);
            std::string algorithm_results = GetJsonFDs(fds);
            std::string results_pyro = pyro.FDAlgorithm::GetJsonFDs();

            EXPECT_EQ(results_pyro, algorithm_results)
                    << "The new algorithm and Pyro yield different results at "
                    << config.path.filename();
        }
    } catch (std::runtime_error& e) {
        std::cout << "Exception raised in test: " << e.what() << std::endl;
        FAIL();
    }
    SUCCEED();
}

}  // namespace tests
