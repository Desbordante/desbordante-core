#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "datasets.h"
#include "depminer.h"
#include "dfd.h"
#include "fastfds.h"
#include "fdep/fdep.h"
#include "fun.h"
#include "hyfd/hyfd.h"
#include "pyro.h"
#include "relational_schema.h"
#include "tane.h"
#include "testing_utils.h"

using ::testing::ContainerEq, ::testing::Eq;
using std::string, std::vector;

namespace fs = std::filesystem;

/* This is a test suite for algorithm verification. It should be possible to run these tests for any algorithm that:
 * 1. extends FDAlgorithm
 * 2. stores the results in FDAlgorithm::fd_collection_
 * 3. has a constructor with signature: myAlgorithm(fs::path const& path, char separator, bool hasHeader)
 *
 * To test your algorithm, just:
 * 1. include the header
 * 2. in CreateAlgorithmInstance replace "Tane" with <your algorithm class name>
 * */

std::vector<unsigned int> BitsetToIndexVector(boost::dynamic_bitset<> const& bitset) {
    std::vector<unsigned int> res;
    for (size_t index = bitset.find_first();
         index != boost::dynamic_bitset<>::npos;
         index = bitset.find_next(index)) {
        res.push_back(index);
    }
    return res;
}

testing::AssertionResult CheckFdListEquality(
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> actual,
    std::list<FD> const& expected) {
    for (auto& fd : expected) {
        std::vector<unsigned int> lhs_indices = BitsetToIndexVector(fd.GetLhs().GetColumnIndices());
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

TYPED_TEST_SUITE_P(AlgorithmTest);

TYPED_TEST_P(AlgorithmTest, ThrowsOnEmpty) {
    auto const path = fs::current_path() / "input_data" / "TestEmpty.csv";
    auto algorithm = TestFixture::CreateAlgorithmInstance(path, ',', true);
    ASSERT_THROW(algorithm->Execute(), std::runtime_error);
}

TYPED_TEST_P(AlgorithmTest, ReturnsEmptyOnSingleNonKey) {
    auto const path = fs::current_path() / "input_data" / "TestSingleColumn.csv";
    auto algorithm = TestFixture::CreateAlgorithmInstance(path, ',', true);
    algorithm->Execute();
    ASSERT_TRUE(algorithm->FdList().empty());
}

TYPED_TEST_P(AlgorithmTest, WorksOnLongDataset) {
    auto const path = fs::current_path() / "input_data" / "TestLong.csv";

    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{{{2}, 1}};

    auto algorithm = TestFixture::CreateAlgorithmInstance(path, ',', true);
    algorithm->Execute();
    ASSERT_TRUE(CheckFdListEquality(true_fd_collection, algorithm->FdList()));
}

TYPED_TEST_P(AlgorithmTest, WorksOnWideDataset) {
    auto const path = fs::current_path() / "input_data" / "TestWide.csv";

    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{
        {{0}, 2}, {{0}, 4}, {{2}, 0}, {{2}, 4}, {{4}, 0}, {{4}, 2}, {{}, 1}, {{}, 3}};

    auto algorithm = TestFixture::CreateAlgorithmInstance(path, ',', true);
    algorithm->Execute();
    ASSERT_TRUE(CheckFdListEquality(true_fd_collection, algorithm->FdList()));
}

TYPED_TEST_P(AlgorithmTest, LightDatasetsConsistentHash) {
    auto const path = fs::current_path() / "input_data";

    try {
        for (auto const& dataset : LightDatasets::datasets_) {
            auto algorithm = TestFixture::CreateAlgorithmInstance(
                path / dataset.name, dataset.separator, dataset.header_presence);
            algorithm->Execute();
            std::cout << dataset.name << std::endl;
            EXPECT_EQ(algorithm->Fletcher16(), dataset.hash)
                << "FD collection hash changed for " << dataset.name;
        }
    }
    catch (std::runtime_error& e) {
        std::cout << "Exception raised in test: " << e.what() << std::endl;
        FAIL();
    }
    SUCCEED();
}

TYPED_TEST_P(AlgorithmTest, HeavyDatasetsConsistentHash) {
    auto const path = fs::current_path() / "input_data";

    try {
        for (auto const& dataset : HeavyDatasets::datasets_) {
            auto algorithm = TestFixture::CreateAlgorithmInstance(
                path / dataset.name, dataset.separator,
                dataset.header_presence);
            algorithm->Execute();
            EXPECT_EQ(algorithm->Fletcher16(), dataset.hash)
                << "The new algorithm and Pyro yield different results at " << dataset.name;
        }
    }
    catch (std::runtime_error& e) {
        std::cout << "Exception raised in test: " << e.what() << std::endl;
        FAIL();
    }
    SUCCEED();
}

REGISTER_TYPED_TEST_SUITE_P(AlgorithmTest, ThrowsOnEmpty, ReturnsEmptyOnSingleNonKey,
                            WorksOnLongDataset, WorksOnWideDataset, LightDatasetsConsistentHash,
                            HeavyDatasetsConsistentHash);

using Algorithms = ::testing::Types<algos::Tane, algos::Pyro, algos::FastFDs, algos::DFD,
                                    algos::Depminer, algos::FDep, algos::FUN, algos::hyfd::HyFD>;
INSTANTIATE_TYPED_TEST_SUITE_P(AlgorithmTest, AlgorithmTest, Algorithms);
