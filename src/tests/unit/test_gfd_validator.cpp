#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/gfd/gfd_validator/gfd_validator.h"
#include "all_gfd_paths.h"
#include "config/names.h"
#include "gfd/gfd_validator/egfd_validator.h"
#include "gfd/gfd_validator/naivegfd_validator.h"

namespace algos {
class GfdHandler;
}  // namespace algos

namespace model {
class Gfd;
}  // namespace model

using namespace algos;
using algos::StdParamsMap;

namespace tests {

namespace {

template <typename T>
class GfdValidatorTest : public ::testing::Test {
protected:
    std::unique_ptr<algos::GfdHandler> CreateGfdValidatorInstance(
            std::filesystem::path const& graph_path,
            std::vector<std::filesystem::path> const& gfd_paths) {
        StdParamsMap option_map = {{config::names::kGraphData, graph_path},
                                   {config::names::kGfdData, gfd_paths}};
        return algos::CreateAndLoadAlgorithm<T>(option_map);
    }
};

TYPED_TEST_SUITE_P(GfdValidatorTest);

TYPED_TEST_P(GfdValidatorTest, TestTrivially) {
    std::vector<std::filesystem::path> gfd_paths = {kGfdTestQuadrangleGfd};
    auto algorithm = TestFixture::CreateGfdValidatorInstance(kGfdTestQuadrangleGraph, gfd_paths);
    int expected_size = 1;
    algorithm->Execute();
    std::vector<model::Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
}

TYPED_TEST_P(GfdValidatorTest, TestExistingMatches) {
    std::vector<std::filesystem::path> gfd_paths = {kGfdTestDirectorsGfd};
    auto algorithm = TestFixture::CreateGfdValidatorInstance(kGfdTestDirectorsGraph, gfd_paths);
    int expected_size = 0;
    algorithm->Execute();
    std::vector<model::Gfd> gfd_list = algorithm->GfdList();
    ASSERT_EQ(expected_size, gfd_list.size());
}

REGISTER_TYPED_TEST_SUITE_P(GfdValidatorTest, TestTrivially, TestExistingMatches);

using GfdAlgorithms =
        ::testing::Types<algos::NaiveGfdValidator, algos::GfdValidator, algos::EGfdValidator>;

INSTANTIATE_TYPED_TEST_SUITE_P(GfdValidatorTest, GfdValidatorTest, GfdAlgorithms);

}  // namespace

}  // namespace tests
