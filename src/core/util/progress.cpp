#include "progress.h"

namespace util {

void Progress::AddProgress(double val) noexcept {
    assert(val >= 0);
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ += val;
    assert(cur_phase_progress_ < 101);
}

void Progress::SetProgress(double val) noexcept {
    assert(0 <= val && val < 101);
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ = val;
}

std::pair<uint8_t, double> Progress::GetProgress() const noexcept {
    std::scoped_lock lock(progress_mutex_);
    return std::make_pair(cur_phase_id_, cur_phase_progress_);
}

void Progress::ResetProgress() noexcept {
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ = 0;
    cur_phase_id_ = 0;
}

void Progress::ToNextProgressPhase() noexcept {
    // Current phase is done, ensure that this is displayed in the progress bar
    SetProgress(kTotalProgressPercent);

    std::scoped_lock lock(progress_mutex_);
    ++cur_phase_id_;
    assert(cur_phase_id_ < phase_names_.size());
    cur_phase_progress_ = 0;
}

}  // namespace util
