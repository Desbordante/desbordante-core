#pragma once

#include <chrono>
#include <ctime>

namespace algos::fastod {

typedef std::chrono::_V2::system_clock::time_point TimePoint;

class Timer {
private:
    TimePoint start_time_;
    TimePoint end_time_;

public:
    Timer();
};

} // namespace algos::fastod
