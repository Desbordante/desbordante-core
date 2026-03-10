#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/fd/eulerfd/eulerfd.h"
#include "core/util/bitset_utils.h"
#include "tests/common/all_csv_configs.h"
#include "tests/unit/test_fd_util.h"

using ::testing::ContainerEq, ::testing::Eq;

namespace tests {
TYPED_TEST_SUITE_P(ApproximateFDTest);

TYPED_TEST_P(ApproximateFDTest, ThrowsOnEmpty) {
    auto algorithm = TestFixture::CreateAndConfToLoad(kTestEmpty);
    ASSERT_THROW(algorithm->LoadData(), std::runtime_error);
}

TYPED_TEST_P(ApproximateFDTest, ReturnsEmptyOnSingleNonKey) {
    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestSingleColumn);
    algorithm->Execute();
    ASSERT_TRUE(NoFDsFound(*algorithm->GetFdStorage()));
}

TYPED_TEST_P(ApproximateFDTest, WorksOnLongDataset) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> true_fd_collection{{{2}, 1}};

    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestLong);
    algorithm->Execute();
    ASSERT_TRUE(CheckFdCollectionEquality(true_fd_collection, *algorithm->GetFdStorage()));
}

TYPED_TEST_P(ApproximateFDTest, WorksOnWideDataset) {
    // unlukely, eulerfd clusters sampler strategy doesnt work on this type of datasets,
    // so answer of eulerfd will be 0
    auto algorithm = TestFixture::CreateAlgorithmInstance(kTestWide);
    algorithm->Execute();
    ASSERT_TRUE(NoFDsFound(*algorithm->GetFdStorage()));
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
    auto first_res = FDsToSet(*algorithm->GetFdStorage());
    for (int i = 0; i < 3; ++i) {
        algos::ConfigureFromMap(*algorithm, TestFixture::GetParamMap(kNeighbors10k));
        algorithm->Execute();
        ASSERT_TRUE(CheckFdCollectionEquality(first_res, *algorithm->GetFdStorage()));
    }
}

REGISTER_TYPED_TEST_SUITE_P(ApproximateFDTest, ThrowsOnEmpty, ReturnsEmptyOnSingleNonKey,
                            WorksOnLongDataset, WorksOnWideDataset, LightDatasetsConsistentHash,
                            HeavyDatasetsConsistentHash, ConsistentRepeatedExecution);

using Algorithms = ::testing::Types<algos::EulerFD>;
INSTANTIATE_TYPED_TEST_SUITE_P(ApproximateFDTest, ApproximateFDTest, Algorithms);
}  // namespace tests
