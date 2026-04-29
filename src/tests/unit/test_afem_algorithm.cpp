#include <filesystem>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fem/afem/afem.h"
#include "core/algorithms/fem/maxfem/model/composite_episode.h"
#include "core/config/names.h"
#include "core/parser/sequence_parser/file_sequence_parser.h"
#include "tests/common/all_sequence_paths.h"

namespace tests {

using ::testing::UnorderedElementsAre;
using ::testing::IsSupersetOf;
using RawEpisode = algos::maxfem::CompositeEpisode::RawEpisode;

class AFEMAlgorithmTest : public ::testing::Test {
protected:
    std::vector<RawEpisode> ExecuteAFEM(std::filesystem::path const& path, size_t minsup,
                                        size_t window_size) {
        using namespace config::names;

        std::shared_ptr<model::ISequenceStream> sequence =
                std::make_shared<parser::FileSequenceParser>(path);

        algos::StdParamsMap param_map = {{kSequence, sequence}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::afem::AFEM>(param_map);

        algo->SetOption(kMinimumSupport, minsup);
        algo->SetOption(kWindowSize, window_size);

        algo->Execute();
        return algo->GetFrequentEpisodes();
    }
};

// Dataset: maxfem_baseline.txt, minsup=2, window=2
//
// Frequent parallel episodes:
//   {1} sup=5, {2} sup=3, {3} sup=2, {1,2} sup=2
//
// All frequent composite episodes (AFEM returns all, MaxFEM only maximal):
//   1-component : [[1]] sup=5, [[2]] sup=3, [[3]] sup=2, [[1,2]] sup=2
//   2-component : [[1],[1]] sup=3, [[1],[2]] sup=2, [[1],[1,2]] sup=2
//   (no 3-component episode meets minsup=2 with window=2)
TEST_F(AFEMAlgorithmTest, Baseline) {
    auto results = ExecuteAFEM(kMaxFemBaselinePath, 2, 2);

    RawEpisode one      = {{{1}},        5};
    RawEpisode two      = {{{2}},        3};
    RawEpisode three    = {{{3}},        2};
    RawEpisode one_two  = {{{1, 2}},     2};
    RawEpisode one_one  = {{{1}, {1}},   3};
    RawEpisode one_2    = {{{1}, {2}},   2};
    RawEpisode one_1two = {{{1}, {1, 2}}, 2};

    EXPECT_THAT(results, UnorderedElementsAre(one, two, three, one_two,
                                              one_one, one_2, one_1two));
}

// Dataset: maxfem_window.txt, minsup=2, window=3
//
// Frequent parallel episodes: {10} sup=4, {11} sup=4, {12} sup=4
//
// All frequent composite episodes:
//   1-component: [[10]] sup=4, [[11]] sup=4, [[12]] sup=4
//   2-component: [[10],[11]] sup=2, [[10],[12]] sup=2, [[11],[12]] sup=2
//   3-component: [[10],[11],[12]] sup=2
TEST_F(AFEMAlgorithmTest, Window) {
    auto results = ExecuteAFEM(kMaxFemWindowPath, 2, 3);

    RawEpisode ten              = {{{10}},               4};
    RawEpisode eleven           = {{{11}},               4};
    RawEpisode twelve           = {{{12}},               4};
    RawEpisode ten_eleven       = {{{10}, {11}},         2};
    RawEpisode ten_twelve       = {{{10}, {12}},         2};
    RawEpisode eleven_twelve    = {{{11}, {12}},         2};
    RawEpisode ten_eleven_twelve = {{{10}, {11}, {12}},  2};

    EXPECT_THAT(results, UnorderedElementsAre(ten, eleven, twelve,
                                              ten_eleven, ten_twelve, eleven_twelve,
                                              ten_eleven_twelve));
}

// For the parallel and pruning datasets the full enumeration is complex, so we
// verify two structural properties:
//   1. MaxFEM results are a subset of AFEM results (maximals are always frequent).
//   2. AFEM finds strictly more episodes than MaxFEM (non-maximal sub-episodes exist).
TEST_F(AFEMAlgorithmTest, ParallelContainsMaxFEM) {
    auto afem_results = ExecuteAFEM(kMaxFemParallelPath, 2, 5);

    RawEpisode aba = {{{10, 11}, {12}, {10, 11}}, 2};
    RawEpisode bab = {{{12}, {10, 11}, {12}},     2};

    EXPECT_THAT(afem_results, IsSupersetOf({aba, bab}));
    EXPECT_GT(afem_results.size(), 2u);
}

TEST_F(AFEMAlgorithmTest, PruningContainsMaxFEM) {
    auto afem_results = ExecuteAFEM(kMaxFemPruningPath, 2, 5);

    RawEpisode ep1 = {{{20}, {21}, {22}},             2};
    RawEpisode ep2 = {{{22}, {10}},                   2};
    RawEpisode ep3 = {{{10}, {11}, {12}, {10}, {11}}, 2};
    RawEpisode ep4 = {{{11}, {12}, {10}, {11}, {12}}, 2};

    EXPECT_THAT(afem_results, IsSupersetOf({ep1, ep2, ep3, ep4}));
    EXPECT_GT(afem_results.size(), 4u);
}

}  // namespace tests
