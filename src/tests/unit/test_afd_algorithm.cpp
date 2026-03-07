#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/fd/depminer/depminer.h"
#include "core/algorithms/fd/dfd/dfd.h"
#include "core/algorithms/fd/fastfds/fastfds.h"
#include "core/algorithms/fd/fdep/fdep.h"
#include "core/algorithms/fd/fun/fun.h"
#include "core/algorithms/fd/hyfd/hyfd.h"
#include "core/algorithms/fd/pyro/pyro.h"
#include "core/algorithms/fd/tane/pfdtane.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/model/table/relational_schema.h"
#include "tests/unit/test_afd_util.h"
#include "tests/unit/test_fd_util.h"

using std::string, std::vector;
using ::testing::ContainerEq, ::testing::Eq;

namespace fs = std::filesystem;

namespace tests {

/* This is a test suite for algorithm verification. It should be possible to run these tests for any
 * algorithm that:
 * 1. extends AFDAlgorithm
 * 2. stores the results in AFDAlgorithm::afd_collection_
 * 3. has a constructor with signature: myAlgorithm(fs::path const& path, char separator, bool
 * has_header)
 *
 * To test your algorithm, just:
 * 1. include the header
 * 2. in CreateAlgorithmInstance replace "Tane" with <your algorithm class name>
 * */

testing::AssertionResult CheckAfdListEquality(
        std::set<std::pair<std::vector<unsigned int>, unsigned int>> actual,
        std::list<AFD> const& expected) {
    for (auto& afd : expected) {
        std::vector<unsigned int> lhs_indices =
                BitsetToIndexVector(afd.GetLhs().GetColumnIndices());
        std::sort(lhs_indices.begin(), lhs_indices.end());

        if (auto it = actual.find(std::make_pair(lhs_indices, afd.GetRhs().GetIndex()));
            it == actual.end()) {
            return testing::AssertionFailure()
                   << "discovered a false FD: " << afd.GetLhs().ToIndicesString() << "->"
                   << afd.GetRhs().ToIndicesString() << " error: " << afd.GetThreshold();
        } else {
            actual.erase(it);
        }
    }
    return actual.empty() ? testing::AssertionSuccess()
                          : testing::AssertionFailure() << "some FDs remain undiscovered";
}

std::set<std::pair<std::vector<unsigned int>, unsigned int>> AFDsToSet(std::list<AFD> const& afds) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> set;
    for (auto const& afd : afds) {
        auto const& raw_afd = afd.ToRawFD();
        set.emplace(BitsetToIndexVector(raw_afd.lhs_), raw_afd.rhs_);
    }
    return set;
}

std::set<std::pair<std::vector<unsigned int>, unsigned int>> FDsToSet(std::list<FD> const& fds) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> set;
    for (auto const& fd : fds) {
        auto const& raw_fd = fd.ToRawFD();
        set.emplace(BitsetToIndexVector(raw_fd.lhs_), raw_fd.rhs_);
    }
    return set;
}

TYPED_TEST_SUITE_P(AlgorithmAFDTest);

TYPED_TEST_P(AlgorithmAFDTest, ThrowsOnEmpty) {
    auto algorithm = TestFixture::CreateAndConfToLoad(kTestEmpty);
    ASSERT_THROW(algorithm->LoadData(), std::runtime_error);
}

TYPED_TEST_P(AlgorithmAFDTest, ReturnsEmptyOnSingleNonKey) {
    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestSingleColumn);
    algorithm->Execute();
    ASSERT_TRUE(algorithm->AfdList().empty());
}

TYPED_TEST_P(AlgorithmAFDTest, WorksOnLongDataset) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{{{2}, 1}};

    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestLong);
    algorithm->Execute();
    ASSERT_TRUE(CheckAfdListEquality(true_fd_collection, algorithm->AfdList()));
}

TYPED_TEST_P(AlgorithmAFDTest, WorksOnWideDataset) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{
            {{0}, 2}, {{0}, 4}, {{2}, 0}, {{2}, 4}, {{4}, 0}, {{4}, 2}, {{}, 1}, {{}, 3}};

    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestWide);
    algorithm->Execute();
    ASSERT_TRUE(CheckAfdListEquality(true_fd_collection, algorithm->AfdList()));
}

TYPED_TEST_P(AlgorithmAFDTest, LightDatasetsConsistentHash) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightDatasets);
}

TYPED_TEST_P(AlgorithmAFDTest, HeavyDatasetsConsistentHash) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyDatasets);
}

TYPED_TEST_P(AlgorithmAFDTest, ConsistentRepeatedExecution) {
    auto algorithm = TestFixture::CreateAlgorithmInstance(kWdcAstronomical);
    algorithm->Execute();
    auto first_res = AFDsToSet(algorithm->AfdList());
    for (int i = 0; i < 3; ++i) {
        algos::ConfigureFromMap(*algorithm, TestFixture::GetParamMap(kWdcAstronomical));
        algorithm->Execute();
        ASSERT_TRUE(CheckAfdListEquality(first_res, algorithm->AfdList()));
    }
}

namespace {
void MaxLhsTestFun(CSVConfig config, std::list<AFD> const& afds_list, config::MaxLhsType max_lhs) {
    using namespace config::names;
    algos::StdParamsMap verify_params = {
            {kCsvConfig, config},
            {kError, config::ErrorType{0.0}},
            {kMaximumLhs, max_lhs},
    };
    auto verify_algo = algos::CreateAndLoadAlgorithm<algos::Pyro>(verify_params);
    verify_algo->Execute();
    auto verify_list = FDsToSet(verify_algo->FdList());
    ASSERT_TRUE(CheckAfdListEquality(verify_list, afds_list));
    for (auto& afd : afds_list) {
        ASSERT_TRUE(afd.GetLhs().GetArity() <= max_lhs);
    }
}
}  // namespace

TYPED_TEST_P(AlgorithmAFDTest, MaxLHSOptionWork) {
    config::MaxLhsType max_lhs = 2;

    auto algo = TestFixture::CreateAlgorithmInstance(kTestFD, max_lhs);
    algo->Execute();
    MaxLhsTestFun(kTestFD, algo->AfdList(), max_lhs);

    auto algo_large = TestFixture::CreateAlgorithmInstance(kCIPublicHighway700, max_lhs);
    algo_large->Execute();
    MaxLhsTestFun(kCIPublicHighway700, algo_large->AfdList(), max_lhs);
}

REGISTER_TYPED_TEST_SUITE_P(AlgorithmAFDTest, MaxLHSOptionWork, ThrowsOnEmpty,
                            ReturnsEmptyOnSingleNonKey, WorksOnLongDataset, WorksOnWideDataset,
                            LightDatasetsConsistentHash, HeavyDatasetsConsistentHash,
                            ConsistentRepeatedExecution);

using AFDAlgorithms = ::testing::Types<algos::Tane, algos::PFDTane>;
INSTANTIATE_TYPED_TEST_SUITE_P(AlgorithmAFDTest, AlgorithmAFDTest, AFDAlgorithms);

}  // namespace tests
