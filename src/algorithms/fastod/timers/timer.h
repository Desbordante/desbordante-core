#pragma once

#include <chrono>

namespace algos::fastod {

typedef std::chrono::_V2::high_resolution_clock::time_point TimePoint;

class Timer {
private:
    bool is_started_;
    TimePoint start_time_;
    TimePoint end_time_;

public:
    Timer(bool start = false) noexcept;

    void Start() noexcept;
    void Stop() noexcept;

    bool IsStarted() const noexcept;
    double GetElapsedSeconds() const;
};

} // namespace algos::fastod
