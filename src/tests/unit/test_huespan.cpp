#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/hue/huespan/high_utility_episode.h"
#include "core/algorithms/hue/huespan/huespan.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "gmock/gmock.h"
#include "tests/common/all_sequence_paths.h"

namespace tests {
using ::testing::UnorderedElementsAre;

class HuespanAlgorithm : public ::testing::Test {
protected:
    std::vector<algos::huespan::HighUtilityEpisode> ExecuteHuespan(
            std::filesystem::path const& path, double min_utility_ratio_, size_t max_dur) {
        algos::StdParamsMap param_map = {{config::names::kUtilitySequence, path}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::huespan::Huespan>(param_map);
        algo->SetOption(config::names::kMinUtilityRatio, min_utility_ratio_);
        algo->SetOption(config::names::kMaxDuration, max_dur);

        algo->Execute();

        return algo->GetHighUtilityEpisodes();
    }
};

TEST_F(HuespanAlgorithm, BasicFirst) {
    auto ep1 = algos::huespan::HighUtilityEpisode({{2, 3}, {1, 3}, {4}}, 15);
    auto ep2 = algos::huespan::HighUtilityEpisode({{2}, {2, 3}, {1, 3}}, 15);
    auto ep3 = algos::huespan::HighUtilityEpisode({{4, 2}, {2, 3}, {1, 3}}, 17);
    auto ep4 = algos::huespan::HighUtilityEpisode({{4}, {2, 3}, {1, 3}}, 15);

    auto hues = ExecuteHuespan(kHuespanBaselinePath, 0.7, 3);
    EXPECT_THAT(hues, UnorderedElementsAre(ep1, ep2, ep3, ep4));
}

TEST_F(HuespanAlgorithm, BasicSecond) {
    auto ep1 = algos::huespan::HighUtilityEpisode({{1}, {4}}, 10);
    auto ep2 = algos::huespan::HighUtilityEpisode({{2, 3}, {1}}, 10);
    auto ep3 = algos::huespan::HighUtilityEpisode({{2, 3}, {1, 3}}, 13);
    auto ep4 = algos::huespan::HighUtilityEpisode({{2}, {1, 3}}, 10);
    auto ep5 = algos::huespan::HighUtilityEpisode({{2}, {3}}, 11);
    auto ep6 = algos::huespan::HighUtilityEpisode({{3}, {1, 3}}, 10);
    auto ep7 = algos::huespan::HighUtilityEpisode({{4, 2}, {2, 3}}, 10);

    auto hues = ExecuteHuespan(kHuespanBaselinePath, 0.45, 2);
    EXPECT_THAT(hues, UnorderedElementsAre(ep1, ep2, ep3, ep4, ep5, ep6, ep7));
};

TEST_F(HuespanAlgorithm, BigDataset) {
    auto ep1 = algos::huespan::HighUtilityEpisode({{456, 66, 304, 882, 445, 753, 724, 508}}, 7383);
    auto ep2 =
            algos::huespan::HighUtilityEpisode({{456, 66, 62, 304, 882, 445, 753, 724, 508}}, 8147);
    auto ep3 = algos::huespan::HighUtilityEpisode({{456, 66, 62, 304, 882, 445, 753, 508}}, 7265);
    auto ep4 = algos::huespan::HighUtilityEpisode({{456, 66, 62, 304, 882, 445, 724, 508}}, 7769);
    auto ep5 = algos::huespan::HighUtilityEpisode({{456, 66, 62, 304, 445, 753, 724, 508}}, 7400);
    auto ep6 = algos::huespan::HighUtilityEpisode({{456, 66, 62, 882, 445, 753, 724, 508}}, 7769);
    auto ep7 = algos::huespan::HighUtilityEpisode({{456, 66, 62, 882, 445, 724, 508}}, 7391);
    auto ep8 = algos::huespan::HighUtilityEpisode({{272}}, 7820);
    auto ep9 = algos::huespan::HighUtilityEpisode({{306}}, 7920);
    auto ep10 = algos::huespan::HighUtilityEpisode({{330}}, 7960);
    auto ep11 = algos::huespan::HighUtilityEpisode({{508}}, 7580);
    auto ep12 = algos::huespan::HighUtilityEpisode({{789}}, 7964);
    auto ep13 = algos::huespan::HighUtilityEpisode({{907}}, 7182);
    auto ep14 = algos::huespan::HighUtilityEpisode({{1254}}, 8074);
    auto ep15 = algos::huespan::HighUtilityEpisode({{1426}}, 11968);

    auto hues = ExecuteHuespan(kHuespanFoodmartPath, 0.0025, 1);
    EXPECT_THAT(hues, UnorderedElementsAre(ep1, ep2, ep3, ep4, ep5, ep6, ep7, ep8, ep9, ep10, ep11,
                                           ep12, ep13, ep14, ep15));
};

TEST_F(HuespanAlgorithm, MultipleExecutions) {
    auto ep1 = algos::huespan::HighUtilityEpisode({{2, 3}, {1, 3}, {4}}, 15);
    auto ep2 = algos::huespan::HighUtilityEpisode({{2}, {2, 3}, {1, 3}}, 15);
    auto ep3 = algos::huespan::HighUtilityEpisode({{4, 2}, {2, 3}, {1, 3}}, 17);
    auto ep4 = algos::huespan::HighUtilityEpisode({{4}, {2, 3}, {1, 3}}, 15);

    algos::StdParamsMap param_map = {{config::names::kUtilitySequence, kHuespanBaselinePath}};
    auto algo = algos::CreateAndLoadAlgorithm<algos::huespan::Huespan>(param_map);
    algo->SetOption(config::names::kMinUtilityRatio, 0.4);
    algo->SetOption(config::names::kMaxDuration, size_t(5));
    algo->Execute();
    auto hues = algo->GetHighUtilityEpisodes();

    algo->SetOption(config::names::kMinUtilityRatio, 0.7);
    algo->SetOption(config::names::kMaxDuration, size_t(3));
    algo->Execute();
    hues = algo->GetHighUtilityEpisodes();

    EXPECT_THAT(hues, UnorderedElementsAre(ep1, ep2, ep3, ep4));
};

TEST_F(HuespanAlgorithm, InvalidSequence) {
    EXPECT_THROW(ExecuteHuespan(kHuespanInvalidSequencePath, 0.5, 3), std::runtime_error);
};

TEST_F(HuespanAlgorithm, InvalidUtilities) {
    EXPECT_THROW(ExecuteHuespan(kHuespanInvalidUtilitiesPath, 0.5, 3), std::runtime_error);
};

TEST_F(HuespanAlgorithm, InvalidMinUtilityParameter) {
    EXPECT_THROW(ExecuteHuespan(kHuespanBaselinePath, 10, 3), config::ConfigurationError);
};

TEST_F(HuespanAlgorithm, InvalidMaxDurationParameter) {
    EXPECT_THROW(ExecuteHuespan(kHuespanBaselinePath, 0.5, 0), config::ConfigurationError);
};

}  // namespace tests