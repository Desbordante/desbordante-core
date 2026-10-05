#include "huespan.h"

#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "utils.h"

namespace algos::huespan {
Huespan::Huespan() : Algorithm() {
    RegisterOption(
            config::Option{&sequence_path_, config::names::kUtilitySequence, "Utility sequence"});
    RegisterOption(config::Option{&min_utility_ratio_, config::names::kMinUtility,
                                  "Minimum utility", 1.0});
    RegisterOption(
            config::Option{&max_duration_, config::names::kMaxDuration, "Maximum duration", 5ul});
    MakeOptionsAvailable({config::names::kUtilitySequence});
}

void Huespan::MakeExecuteOptsAvailable() {
    Algorithm::MakeExecuteOptsAvailable();
    MakeOptionsAvailable({config::names::kMinUtility, config::names::kMaxDuration});
}

void Huespan::ResetState() {}

void Huespan::ScanFile() {
    std::ifstream file(sequence_path_);
    std::string single_episode;
    int current_timepoint = 1;

    while (getline(file, single_episode)) {
        auto single_episode_splitted = util::SplitString(single_episode, ':');
        auto events = util::SplitString(single_episode_splitted[0], ' ');
        int total_utility = std::stoi(single_episode_splitted[1]);
        auto utilities = util::SplitString(single_episode_splitted[2], ' ');

        for (int i = 0; i < events.size(); ++i) {
            int event = std::stoi(events[i]);
            int event_utility = std::stoi(utilities[i]);
            sequence_.AddEvent(current_timepoint, event, event_utility);

            single_candidates_minimal_occurs_utility_[event].Add(current_timepoint, event_utility);
        }
        current_timepoint++;
        sequence_utility_ += total_utility;
    }
}

std::pair<int, int> Huespan::CalculateMOsUtilityAndUpperBound(
        int beta, std::vector<int> const& beta_start_timepoints,
        std::vector<int> const& beta_end_timepoints, std::vector<int> beta_prev_utility,
        std::vector<int> beta_utility) {
    int total_utilty = 0;
    int upper_bound = 0;
    int previous_start_timepoint = -1;
    for (int pos = 0; pos < beta_start_timepoints.size(); ++pos) {
        int start_timepoint = beta_start_timepoints[pos];
        if (start_timepoint == previous_start_timepoint) continue;

        previous_start_timepoint = start_timepoint;
        total_utilty += beta_utility[pos];
        int end_timepoint = beta_end_timepoints[pos];

        upper_bound += beta_utility[pos] + sequence_.GetRemainingUtility(end_timepoint, beta) +
                       sequence_.GetUtilityOfDuration(end_timepoint + 1,
                                                      start_timepoint + max_duration_ - 1);
    }
    return {total_utilty, upper_bound};
}

void Huespan::BuildCoocurenceMatrix() {
    for (int timepoint = 1; timepoint <= sequence_.GetLastTimepoint(); ++timepoint) {
        auto pairs = sequence_.GetTimepointEvents(timepoint);
        std::unordered_set<int> processed_simult;
        for (auto const& pair : pairs) {
            int event = pair.first;

            int utility_simult = sequence_.GetUtilityOfDuration(timepoint - max_duration_ + 1,
                                                                timepoint + max_duration_ - 1);
            for (int event_j : processed_simult) {
                cooccurence_matrix_simult_[event_j][event] += utility_simult;
            }
            processed_simult.insert(event);

            std::unordered_set<int> processed_serial;
            for (int timepoint_serial = timepoint + 1;
                 timepoint_serial <= timepoint + max_duration_ - 1; ++timepoint_serial) {
                auto pairs_serial = sequence_.GetTimepointEvents(timepoint_serial);
                for (auto const& pair_serial : pairs_serial) {
                    int event_serial = pair_serial.first;
                    if (processed_serial.count(event_serial)) continue;

                    int utility_serial = sequence_.GetUtilityOfDuration(
                            timepoint_serial - max_duration_ + 1, timepoint + max_duration_ - 1);
                    cooccurence_matrix_serial_[event][event_serial] += utility_serial;
                    processed_serial.insert(event_serial);
                }
            }
        }
    }
}

void Huespan::MineSimultHUE(std::vector<std::vector<int>>& alpha_episode,
                       std::vector<int>& alpha_start_timepoints,
                       std::vector<int>& alpha_end_timepoints,
                       std::vector<int>& alpha_prev_utilities, std::vector<int>& alpha_utilities) {
        std::unordered_map<int, std::array<std::vector<int>, 4>> beta_info;
        std::unordered_set<int> pruning_set;
        auto const& last_eventset = alpha_episode.back();
        int last_event = last_eventset.back();

        for (int i = 0; i < alpha_start_timepoints.size(); ++i) {
            int start_timepoint = alpha_start_timepoints[i];
            int end_timepoint = alpha_end_timepoints[i];
            int alpha_prev_utility = alpha_prev_utilities[i];
            int alpha_utility = alpha_utilities[i];

            auto extension_pairs = sequence_.GetExtentionEvents(end_timepoint, last_event);

            for (auto const& extention_pair : extension_pairs) {
                int beta = extention_pair.first;
                int beta_utility = alpha_utility + extention_pair.second;

                if (pruning_set.count(beta)) continue;

                bool pruned = false;
                for (int event : last_eventset) {
                    auto it = cooccurence_matrix_simult_.find(event);
                    if (it == cooccurence_matrix_simult_.end()) continue;
                    auto it2 = it->second.find(beta);
                    if (it2 == it->second.end() || it2->second < min_utility_absolute_) {
                        pruning_set.insert(beta);
                        pruned = true;
                        break;
                    }
                }
                if (pruned) continue;

                for (int i = 0; i < alpha_episode.size() - 1 && !pruned; ++i) {
                    for (int event : alpha_episode[i]) {
                        auto it = cooccurence_matrix_serial_.find(event);
                        if (it == cooccurence_matrix_serial_.end()) continue;
                        auto it2 = it->second.find(beta);
                        if (it2 == it->second.end() || it2->second < min_utility_absolute_) {
                            pruning_set.insert(beta);
                            pruned = true;
                            break;
                        }
                    }
                }
                if (pruned) continue;
                auto& info = beta_info[beta];
                info[0].push_back(start_timepoint);
                info[1].push_back(end_timepoint);
                info[2].push_back(alpha_prev_utility);
                info[3].push_back(beta_utility);
            }
        }

        for (auto& [beta, info] : beta_info) {
            auto [total_utility, upper_bound] =
                    CalculateMOsUtilityAndUpperBound(beta, info[0], info[1], info[2], info[3]);

            if (upper_bound >= min_utility_absolute_) {
                std::vector<int> new_last_eventset = last_eventset;
                new_last_eventset.push_back(beta);
                std::vector<std::vector<int>> beta_episode(alpha_episode.begin(),
                                                           alpha_episode.end() - 1);
                beta_episode.push_back(new_last_eventset);

                if (total_utility >= min_utility_absolute_) {
                    HighUtilityEpisode hue(beta_episode, total_utility);
                    high_utility_episodes_.push_back(hue);
                }
                MineHUE(beta_episode, info[0], info[1], info[2], info[3]);
            }
        }
    }
void Huespan::MineSerialHUE(std::vector<std::vector<int>>& alpha_episode,
                       std::vector<int>& alpha_start_timepoints,
                       std::vector<int>& alpha_end_timepoints,
                       std::vector<int>& alpha_prev_utilities, std::vector<int>& alpha_utilities) {
        std::unordered_map<int, std::array<std::vector<int>, 4>> beta_info;
        std::unordered_set<int> pruning_set;
        auto const& last_eventset = alpha_episode.back();
        int previous_start_timepoint = -1;
        for (int i = 0; i < alpha_end_timepoints.size(); ++i) {
            int start_timepoint = alpha_start_timepoints[i];
            if (start_timepoint == previous_start_timepoint) continue;

            previous_start_timepoint = start_timepoint;
            int end_timepoint = alpha_end_timepoints[i];
            int alpha_utility = alpha_utilities[i];

            int j = i + 1;
            for (; j < alpha_end_timepoints.size(); ++j) {
                if (alpha_start_timepoints[j] != start_timepoint) break;
            }

            int next_mo_end_timepoint = sequence_.GetLastTimepoint();
            if (j < alpha_end_timepoints.size()) {
                next_mo_end_timepoint = alpha_end_timepoints[j];
            }
            int extention_bound = std::min(next_mo_end_timepoint,
                                           start_timepoint + static_cast<int>(max_duration_) - 1);

            int max_alpha_utility = alpha_utility;
            for (int timepoint = end_timepoint + 1; timepoint <= extention_bound; ++timepoint) {
                for (auto const& pair : sequence_.GetTimepointEvents(timepoint)) {
                    int beta = pair.first;

                    if (alpha_episode.size() >= 2) {
                        max_alpha_utility = sequence_.GetMaximallUtility(
                                alpha_episode, start_timepoint, timepoint - 1);
                    }
                    int beta_utility = max_alpha_utility + pair.second;

                    if (pruning_set.contains(beta)) continue;

                    bool pruned = false;
                    for (int event : last_eventset) {
                        auto it = cooccurence_matrix_serial_.find(event);
                        if (it == cooccurence_matrix_serial_.end()) continue;
                        auto it2 = it->second.find(beta);
                        if (it2 == it->second.end() || it2->second < min_utility_absolute_) {
                            pruning_set.insert(beta);
                            pruned = true;
                            break;
                        }
                    }
                    if (pruned) continue;

                    for (int i = 0; i < alpha_episode.size() - 1 && !pruned; ++i) {
                        for (int event : alpha_episode[i]) {
                            auto it = cooccurence_matrix_serial_.find(event);
                            if (it == cooccurence_matrix_serial_.end()) continue;
                            auto it2 = it->second.find(beta);
                            if (it2 == it->second.end() || it2->second < min_utility_absolute_) {
                                pruning_set.insert(beta);
                                pruned = true;
                                break;
                            }
                        }
                    }
                    if (pruned) continue;

                    auto it = beta_info.find(beta);
                    if (it == beta_info.end()) {
                        beta_info[beta];
                    }
                    auto& info = beta_info[beta];
                    info[0].push_back(start_timepoint);
                    info[1].push_back(timepoint);
                    info[2].push_back(alpha_utility);
                    info[3].push_back(beta_utility);
                }
            }
        }

        for (auto& [beta, info] : beta_info) {
            auto [total_utility, upper_bound] =
                    CalculateMOsUtilityAndUpperBound(beta, info[0], info[1], info[2], info[3]);
            if (upper_bound >= min_utility_absolute_) {
                std::vector<std::vector<int>> beta_episode = alpha_episode;
                beta_episode.push_back({beta});
                if (total_utility >= min_utility_absolute_) {
                    HighUtilityEpisode hue(beta_episode, total_utility);
                    high_utility_episodes_.push_back(hue);
                }
                MineHUE(beta_episode, info[0], info[1], info[2], info[3]);
            }
        }
    }
void Huespan::ExecuteInternal() {
        min_utility_absolute_ = min_utility_ratio_ * sequence_utility_;
        sequence_.PruneSingleEvents(max_duration_, min_utility_absolute_,
                                    single_candidates_minimal_occurs_utility_);

        BuildCoocurenceMatrix();

        for (auto& [candidate, moutility] : single_candidates_minimal_occurs_utility_) {
            std::vector<std::vector<int>> alpha_episode = {{candidate}};
            std::vector<int> alpha_mos = moutility.GetMinOccurencies();
            std::vector<int> alpha_utilities = moutility.GetUtilities();
            std::vector<int> alpha_prev_utility(alpha_mos.size(), 0);

            auto [total_utility, upper_bound] = CalculateMOsUtilityAndUpperBound(
                    candidate, alpha_mos, alpha_mos, alpha_prev_utility, alpha_utilities);

            if (upper_bound >= min_utility_absolute_) {
                if (total_utility >= min_utility_absolute_) {
                    HighUtilityEpisode hue(alpha_episode, total_utility);
                    high_utility_episodes_.push_back(hue);
                }
                MineHUE(alpha_episode, alpha_mos, alpha_mos, alpha_prev_utility, alpha_utilities);
            }
        }
    };
}  // namespace algos::huespan