#include <algorithm>
#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/fd_mine/fd_mine.h"
#include "core/algorithms/fd/pyro/pyro.h"
#include "core/algorithms/fd/tane/pfdtane.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/config/error/type.h"
#include "core/config/names.h"
#include "core/model/table/relational_schema.h"
#include "tests/common/csv_config_util.h"
#include "tests/unit/test_fd_util.h"

namespace tests {
using ::testing::ContainerEq, ::testing::Eq;

using algos::Algorithm, algos::FdMine, algos::StdParamsMap;

namespace onam = config::names;

StdParamsMap FdMineGetParamMap(CSVConfig const& csv_config) {
    return {{config::names::kTable, MakeInputTable(csv_config)}};
}

std::unique_ptr<Algorithm> ConfToLoadFdMine(CSVConfig const& csv_config) {
    std::unique_ptr<Algorithm> algorithm = std::make_unique<FdMine>();
    algos::ConfigureFromMap(*algorithm, FdMineGetParamMap(csv_config));
    return algorithm;
}

std::unique_ptr<FdMine> CreateFdMineAlgorithmInstance(CSVConfig const& csv_config) {
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
    ASSERT_TRUE(NoFDsFound(*algorithm->GetFdStorage()));
}

TEST(AlgorithmSyntheticTest, FD_Mine_WorksOnLongDataset) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{{{2}, 1}};

    auto algorithm = CreateFdMineAlgorithmInstance(tests::kTestLong);
    algorithm->Execute();
    ASSERT_TRUE(CheckFdCollectionEquality(true_fd_collection, *algorithm->GetFdStorage()));
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

void MinimizeFDs(algos::MultiAttrRhsFdStorage::Storage& fd_collection) {
    algos::MultiAttrRhsFdStorage::Storage::iterator it1 = fd_collection.begin();
    while (it1 != fd_collection.end()) {
        algos::MultiAttrRhsFdStorage::Storage::iterator it2 = fd_collection.begin();
        while (it2 != fd_collection.end()) {
            if (it1 == it2) {
                it2++;
                continue;
            }
            if (it1->lhs.is_subset_of(it2->lhs)) {
                it2->rhs -= it1->rhs;
                if (it2->rhs.none()) {
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
            algos::MultiAttrRhsFdStorage const& fd_storage = *algorithm->GetFdStorage();
            algos::MultiAttrRhsFdStorage::Storage fds = algorithm->GetFdStorage()->GetStripped();
            pyro.Execute();

            for (auto& fd : pyro.FdList()) {
                if (fd.GetLhs().GetArity() == 0) {
                    auto it = fds.begin();
                    while (it != fds.end()) {
                        if (it->rhs.test(fd.GetRhs().GetIndex())) {
                            it->rhs.reset(fd.GetRhs().GetIndex());
                            if (it->rhs.none()) {
                                it = fds.erase(it);
                                continue;
                            }
                        }
                        it++;
                    }
                    boost::dynamic_bitset<> rhs(fd_storage.GetTableHeader().column_names.size());
                    rhs.set(fd.GetRhs().GetIndex());
                    fds.emplace_back(fd.GetLhs().GetColumnIndices(), std::move(rhs));
                }
            }

            MinimizeFDs(fds);
            std::string algorithm_results = FDsToJson({fd_storage.GetTableHeader(), fds});
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
