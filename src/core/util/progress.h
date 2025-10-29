#pragma once

#include <cstdint>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

namespace util {

// Describes a progress of a primitive mining algorithm.
// Progress of the algorithm is represented as a set of phases each of which has separate double
// value between 0 and 100. This value descibes progress of specific phase in natural way:
// 0 - nothing done yet, 100 - the phase is finished and each value in between represents the
// percentage of the work associated with the phase done.
class Progress {
private:
    std::mutex mutable progress_mutex_;
    double cur_phase_progress_ = 0;
    uint8_t cur_phase_id_ = 0;
    std::vector<std::string_view> phase_names_;

public:
    constexpr static double kTotalProgressPercent = 100.0;

    explicit Progress(std::vector<std::string_view> phase_names)
        : phase_names_(std::move(phase_names)) {}

    // Adds value to the progress of the current phase
    void AddProgress(double val) noexcept;
    // Sets value of the progress of the current phase
    void SetProgress(double val) noexcept;
    // Resets current phase and its progress to zeros
    void ResetProgress() noexcept;
    // Returns pair with current progress state.
    // Pair has the form <current phase id, current phase progess>
    std::pair<uint8_t, double> GetProgress() const noexcept;
    // Finishes current phase and moves to the next one
    void ToNextProgressPhase() noexcept;

    std::vector<std::string_view> const& GetPhaseNames() const noexcept {
        return phase_names_;
    }
};

}  // namespace util
