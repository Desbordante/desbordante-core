#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/fd/eulerfd/eulerfd.h"
#include "all_csv_configs.h"
#include "test_fd_util.h"
#include "util/bitset_utils.h"

using ::testing::ContainerEq, ::testing::Eq;

namespace tests {

static testing::AssertionResult CheckFdListEquality(
        std::set<std::pair<std::vector<unsigned int>, unsigned int>> actual,
        std::list<FD> const& expected) {
    for (auto& fd : expected) {
        std::vector<unsigned int> lhs_indices =
                util::BitsetToIndices<unsigned int>(fd.GetLhs().GetColumnIndices());
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

TYPED_TEST_SUITE_P(ApproximateFDTest);

TYPED_TEST_P(ApproximateFDTest, ThrowsOnEmpty) {
    auto algorithm = TestFixture::CreateAndConfToLoad(kTestEmpty);
    ASSERT_THROW(algorithm->LoadData(), std::runtime_error);
}

TYPED_TEST_P(ApproximateFDTest, ReturnsEmptyOnSingleNonKey) {
    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestSingleColumn);
    algorithm->Execute();
    ASSERT_TRUE(algorithm->FdList().empty());
}

TYPED_TEST_P(ApproximateFDTest, WorksOnLongDataset) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{{{2}, 1}};

    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestLong);
    algorithm->Execute();
    ASSERT_TRUE(CheckFdListEquality(true_fd_collection, algorithm->FdList()));
}

TYPED_TEST_P(ApproximateFDTest, WorksOnWideDataset) {
    // unlukely, eulerfd clusters sampler strategy doesnt work on this type of datasets,
    // so answer of eulerfd will be 0
    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestWide);
    algorithm->Execute();
    ASSERT_TRUE(algorithm->FdList().empty());
}

TYPED_TEST_P(ApproximateFDTest, LightDatasetsConsistentHash) {
    TestFixture::PerformConsistentHashTestOn(
            ApproximateDatasets<typename TestFixture::AlgorithmType>::kLightDatasets);
}

TYPED_TEST_P(ApproximateFDTest, HeavyDatasetsConsistentHash) {
    TestFixture::PerformConsistentHashTestOn(
            ApproximateDatasets<typename TestFixture::AlgorithmType>::kHeavyDatasets);
}

TYPED_TEST_P(ApproximateFDTest, ConsistentRepeatedExecution) {
    auto algorithm = TestFixture::CreateAlgorithmInstance(kNeighbors10k);
    algorithm->Execute();
    auto first_res = FDsToSet(algorithm->FdList());
    for (int i = 0; i < 3; ++i) {
        algos::ConfigureFromMap(*algorithm, TestFixture::GetParamMap(kNeighbors10k));
        algorithm->Execute();
        ASSERT_TRUE(CheckFdListEquality(first_res, algorithm->FdList()));
    }
}

REGISTER_TYPED_TEST_SUITE_P(ApproximateFDTest, ThrowsOnEmpty, ReturnsEmptyOnSingleNonKey,
                            WorksOnLongDataset, WorksOnWideDataset, LightDatasetsConsistentHash,
                            HeavyDatasetsConsistentHash, ConsistentRepeatedExecution);

using Algorithms = ::testing::Types<algos::EulerFD>;
INSTANTIATE_TYPED_TEST_SUITE_P(ApproximateFDTest, ApproximateFDTest, Algorithms);
}  // namespace tests
