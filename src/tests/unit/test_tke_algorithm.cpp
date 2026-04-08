#include <filesystem>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fem/tke/tke.h"
#include "core/config/names.h"
#include "core/parser/sequence_parser/file_sequence_parser.h"
#include "tests/common/all_sequence_paths.h"

namespace tests {

using ::testing::Contains;

class TKEAlgorithmTest : public ::testing::Test {
protected:
    std::vector<algos::tke::CompositeEpisode::RawEpisode> ExecuteTKE(
            std::filesystem::path const& path, size_t k, size_t window_size) {
        using namespace config::names;

        std::shared_ptr<model::ISequenceStream> sequence =
                std::make_shared<parser::FileSequenceParser>(path);

        algos::StdParamsMap param_map = {{kSequence, sequence}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::tke::TKE>(param_map);

        algo->SetOption(kEpisodesNum, k);
        algo->SetOption(kWindowSize, window_size);

        algo->Execute();
        return algo->GetTopKFrequentEpisodes();
    }
};

TEST_F(TKEAlgorithmTest, TopOneIncludesMostFrequentSingleton) {
    auto results = ExecuteTKE(kMaxFemBaselinePath, 1, 10);
    algos::tke::CompositeEpisode::RawEpisode const singleton = {{{1}}, 5};
    EXPECT_THAT(results, Contains(singleton));
    for (auto const& ep : results) {
        EXPECT_EQ(ep.second, 5);
    }
}

TEST_F(TKEAlgorithmTest, TopTwoBaseline) {
    auto results = ExecuteTKE(kMaxFemBaselinePath, 2, 2);
    EXPECT_FALSE(results.empty());
}

}  // namespace tests
