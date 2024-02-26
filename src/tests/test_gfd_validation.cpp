#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/gfd/gfd_validation.h"
#include "config/names.h"
#include "csv_config_util.h"

using namespace algos;
using algos::StdParamsMap;

namespace tests {

namespace {

auto current_path = test_data_dir / "graph_data";

template <typename T>
class GfdValidationTest : public ::testing::Test {
protected:
    std::unique_ptr<algos::GfdHandler> CreateGfdValidationInstance(
            std::filesystem::path const& graph_path,
            std::vector<std::filesystem::path> const& gfd_paths) {
        StdParamsMap optionMap = {{config::names::kGraphData, graph_path},
                                  {config::names::kGfdData, gfd_paths}};
        return algos::CreateAndLoadAlgorithm<T>(optionMap);
    }
};

TYPED_TEST_SUITE_P(GfdValidationTest);

TYPED_TEST_P(GfdValidationTest, TestTrivially) {
    auto graph_path = current_path / "quadrangle.dot";
    auto gfd_path = current_path / "quadrangle_gfd.dot";
    std::vector<std::filesystem::path> gfd_paths = {gfd_path};
    auto algorithm = TestFixture::CreateGfdValidationInstance(graph_path, gfd_paths);
    int expected_size = 1;
    algorithm->Execute();
    std::vector<Gfd> GfdList = algorithm->GfdList();
    ASSERT_EQ(expected_size, GfdList.size());
}

TYPED_TEST_P(GfdValidationTest, TestExistingMatches) {
    auto graph_path = current_path / "directors.dot";
    auto gfd_path = current_path / "directors_gfd.dot";
    std::vector<std::filesystem::path> gfd_paths = {gfd_path};
    auto algorithm = TestFixture::CreateGfdValidationInstance(graph_path, gfd_paths);
    int expected_size = 0;
    algorithm->Execute();
    std::vector<Gfd> GfdList = algorithm->GfdList();
    ASSERT_EQ(expected_size, GfdList.size());
}

REGISTER_TYPED_TEST_SUITE_P(GfdValidationTest, TestTrivially, TestExistingMatches);

using GfdAlgorithms =
        ::testing::Types<algos::NaiveGfdValidation, algos::GfdValidation, algos::EGfdValidation>;

INSTANTIATE_TYPED_TEST_SUITE_P(GfdValidationTest, GfdValidationTest, GfdAlgorithms);

}  // namespace

}  // namespace tests
