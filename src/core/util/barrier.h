#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace util {
template <typename Completion>
class Barrier {
    std::size_t const thread_num_;
    std::size_t count_ = thread_num_;
    std::size_t phase_ = 0;
    Completion completion_;
    std::condition_variable next_phase_var_;
    std::mutex mt_;

    std::size_t PhaseDo(auto action) {
        std::lock_guard lk{mt_};
        return action(phase_);
    }

public:
    Barrier(std::size_t expected, Completion completion)
        : thread_num_(expected), completion_(std::move(completion)) {}

    std::size_t Arrive(std::size_t n = 1) {
        std::unique_lock lk{mt_};
        std::size_t cur_count = (count_ -= n);
        if (cur_count == 0) {
            completion_();
            count_ = thread_num_;
            std::size_t old_phase = phase_++;
            lk.unlock();
            next_phase_var_.notify_all();
            return old_phase;
        } else {
            return phase_;
        }
    }

    void Wait(std::size_t phase) {
        std::unique_lock<std::mutex> lk{mt_};
        next_phase_var_.wait(lk, [this, phase]() { return phase_ != phase; });
    }

    void ArriveAndWait() {
        Wait(Arrive());
    }
};
}  // namespace util
