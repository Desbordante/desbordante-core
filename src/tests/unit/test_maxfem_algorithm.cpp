#include <filesystem>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fem/maxfem/maxfem.h"
#include "core/config/names.h"
#include "core/parser/sequence_parser/file_sequence_parser.h"
#include "tests/common/all_sequence_paths.h"

namespace tests {

using ::testing::ContainerEq;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::UnorderedElementsAre;

class MaxFEMAlgorithmTest : public ::testing::Test {
protected:
    std::vector<algos::maxfem::CompositeEpisode::RawEpisode> ExecuteMaxFEM(
            std::filesystem::path const& path, size_t minsup, size_t window_size) {
        using namespace config::names;

        std::shared_ptr<model::ISequenceStream> sequence =
                std::make_shared<parser::FileSequenceParser>(path);

        algos::StdParamsMap param_map = {{kSequence, sequence}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::maxfem::MaxFEM>(param_map);

        algo->SetOption(kMinimumSupport, minsup);
        algo->SetOption(kWindowSize, window_size);

        algo->Execute();
        return algo->GetMaxFrequentEpisodes();
    }
};

TEST_F(MaxFEMAlgorithmTest, Baseline) {
    auto results = ExecuteMaxFEM(kMaxFemBaselinePath, 2, 2);
    algos::maxfem::CompositeEpisode::RawEpisode ep1 = {{3}};
    algos::maxfem::CompositeEpisode::RawEpisode ep2 = {{1}, {1, 2}};

    EXPECT_THAT(results, UnorderedElementsAre(ep1, ep2));
}

TEST_F(MaxFEMAlgorithmTest, WindowTest) {
    auto results = ExecuteMaxFEM(kMaxFemWindowPath, 2, 3);
    algos::maxfem::CompositeEpisode::RawEpisode expected = {{10}, {11}, {12}};

    EXPECT_THAT(results, UnorderedElementsAre(expected));
}

TEST_F(MaxFEMAlgorithmTest, Parallel) {
    auto results = ExecuteMaxFEM(kMaxFemParallelPath, 2, 5);
    algos::maxfem::CompositeEpisode::RawEpisode aba = {{10, 11}, {12}, {10, 11}};
    algos::maxfem::CompositeEpisode::RawEpisode bab = {{12}, {10, 11}, {12}};

    EXPECT_THAT(results, UnorderedElementsAre(aba, bab));
}

TEST_F(MaxFEMAlgorithmTest, Pruning) {
    auto results_pruning = ExecuteMaxFEM(kMaxFemPruningPath, 2, 5);
    algos::maxfem::CompositeEpisode::RawEpisode ep1 = {{20}, {21}, {22}};
    algos::maxfem::CompositeEpisode::RawEpisode ep2 = {{22}, {10}};
    algos::maxfem::CompositeEpisode::RawEpisode ep3 = {{10}, {11}, {12}, {10}, {11}};
    algos::maxfem::CompositeEpisode::RawEpisode ep4 = {{11}, {12}, {10}, {11}, {12}};

    EXPECT_THAT(results_pruning, UnorderedElementsAre(ep1, ep2, ep3, ep4));
}

}  // namespace tests
