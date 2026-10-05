#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "complex_util_sequence.h"
#include "core/algorithms/algorithm.h"
#include "high_utility_episode.h"

namespace algos::huespan {
class Huespan : public Algorithm {
private:
    double min_utility_ratio_;
    double min_utility_absolute_;
    size_t sequence_utility_ = 0;

    size_t max_duration_;

    std::filesystem::path sequence_path_;

    ComplexSequence sequence_;

    std::unordered_map<int, std::unordered_map<int, int>> cooccurence_matrix_simult_;
    std::unordered_map<int, std::unordered_map<int, int>> cooccurence_matrix_serial_;

    std::unordered_map<int, EpisodeMinimalOccsUtilities> single_candidates_minimal_occurs_utility_;

    std::vector<HighUtilityEpisode> high_utility_episodes_;

    void BuildCoocurenceMatrix();

    std::pair<int, int> CalculateMOsUtilityAndUpperBound(
            int beta, std::vector<int> const& beta_start_timepoints,
            std::vector<int> const& beta_end_timepoints, std::vector<int> beta_prev_utility,
            std::vector<int> beta_utility);

    void MineHUE(std::vector<std::vector<int>>& alpha_episode,
                 std::vector<int>& alpha_start_timepoints, std::vector<int>& alpha_end_timepoints,
                 std::vector<int>& alpha_prev_utilities, std::vector<int>& alpha_utilities) {
        MineSimultHUE(alpha_episode, alpha_start_timepoints, alpha_end_timepoints,
                      alpha_prev_utilities, alpha_utilities);
        MineSerialHUE(alpha_episode, alpha_start_timepoints, alpha_end_timepoints,
                      alpha_prev_utilities, alpha_utilities);
    }

    void MineSimultHUE(std::vector<std::vector<int>>& alpha_episode,
                       std::vector<int>& alpha_start_timepoints,
                       std::vector<int>& alpha_end_timepoints,
                       std::vector<int>& alpha_prev_utilities, std::vector<int>& alpha_utilities);

    void MineSerialHUE(std::vector<std::vector<int>>& alpha_episode,
                       std::vector<int>& alpha_start_timepoints,
                       std::vector<int>& alpha_end_timepoints,
                       std::vector<int>& alpha_prev_utilities, std::vector<int>& alpha_utilities);

    void LoadDataInternal() override {
        ScanFile();
    }

    void ResetState() override;

    void ExecuteInternal() override;

    void ScanFile();

protected:
    void MakeExecuteOptsAvailable() override;

public:
    std::vector<HighUtilityEpisode> const& GetHighUtilityEpisodes() const {
        return high_utility_episodes_;
    }

    Huespan();
};
}  // namespace algos::huespan