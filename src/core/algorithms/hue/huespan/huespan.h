#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "complex_utility_sequence.h"
#include "core/algorithms/algorithm.h"
#include "high_utility_episode.h"
#include "minimal_occurences.h"

namespace algos::huespan {
class Huespan : public Algorithm {
private:
    double min_utility_ratio_;
    double min_utility_absolute_;
    size_t sequence_initial_utility_ = 0;

    size_t max_duration_;

    std::filesystem::path sequence_path_;

    ComplexSequence sequence_;
    ComplexSequence pruned_sequence_;

    std::unordered_map<int, std::unordered_map<int, int>> cooccurence_matrix_simult_;
    std::unordered_map<int, std::unordered_map<int, int>> cooccurence_matrix_serial_;

    std::unordered_map<int, EventMinimalOccurences> single_candidates_mos_;

    std::vector<HighUtilityEpisode> high_utility_episodes_;

    void BuildCoocurenceMatrix();

    std::pair<int, int> CalculateMOsUtilityAndUpperBound(
            int beta, EpisodeMinimalOccurences const& occurences);

    void MineHUE(std::vector<std::vector<int>> const& alpha_episode,
                 EpisodeMinimalOccurences const& occurences);

    void MineSimultHUE(std::vector<std::vector<int>> const& alpha_episode,
                       EpisodeMinimalOccurences const& occurences);

    void MineSerialHUE(std::vector<std::vector<int>> const& alpha_episode,
                       EpisodeMinimalOccurences const& occurences);

    void LoadDataInternal() override;
    void ResetState() override;
    void ExecuteInternal() override;
    void ScanFile();
    void RegisterOptions();

protected:
    void MakeExecuteOptsAvailable() override;

public:
    std::vector<HighUtilityEpisode> const& GetHighUtilityEpisodes() const {
        return high_utility_episodes_;
    }

    Huespan();
};
}  // namespace algos::huespan