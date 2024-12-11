#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/gfd/gfd_validation.h"
#include "all_paths.h"
#include "config/names.h"

using namespace algos;
using algos::StdParamsMap;

namespace tests {

namespace {

template <typename T>
class GfdValidationTest : public ::testing::Test {
protected:
    std::unique_ptr<algos::GfdHandler> CreateGfdValidationInstance(
            std::filesystem::path const& graph_path,
            std::vector<std::filesystem::path> const& gfd_paths) {
        StdParamsMap option_map = {{config::names::kGraphData, graph_path},
                                   {config::names::kGfdData, gfd_paths}};
        return algos::CreateAndLoadAlgorithm<T>(option_map);
    }
};

TYPED_TEST_SUITE_P(GfdValidationTest);

TYPED_TEST_P(GfdValidationTest, TestTrivially) {
    std::vector<std::filesystem::path> gfd_paths = {kGfdTestQuadrangleGfd};
    auto algorithm = TestFixture::CreateGfdValidationInstance(kGfdTestQuadrangleGraph, gfd_paths);
    int expected_size = 1;
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
}

TYPED_TEST_P(GfdValidationTest, TestExistingMatches) {
    std::vector<std::filesystem::path> gfd_paths = {kGfdTestDirectorsGfd};
    auto algorithm = TestFixture::CreateGfdValidationInstance(kGfdTestDirectorsGraph, gfd_paths);
    int expected_size = 0;
    algorithm->Execute();
    std::vector<Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
}

REGISTER_TYPED_TEST_SUITE_P(GfdValidationTest, TestTrivially, TestExistingMatches);

using GfdAlgorithms =
        ::testing::Types<algos::NaiveGfdValidation, algos::GfdValidation, algos::EGfdValidation>;

INSTANTIATE_TYPED_TEST_SUITE_P(GfdValidationTest, GfdValidationTest, GfdAlgorithms);

}  // namespace

}  // namespace tests
