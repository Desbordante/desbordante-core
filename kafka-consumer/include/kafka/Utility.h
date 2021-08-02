#pragma once

#include "kafka/Project.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <time.h>


namespace KAFKA_API {

namespace Utility {

/**
 * Get local time as string.
 */
inline std::string getLocalTimeString(const std::chrono::system_clock::time_point& timePoint)
{
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(timePoint.time_since_epoch()) % 1000000;
    auto time = std::chrono::system_clock::to_time_t(timePoint);

    std::ostringstream oss;
    std::tm tmBuf = {};
#if !defined(WIN32)
    oss << std::put_time(localtime_r(&time, &tmBuf), "%F %T") <<  "." << std::setfill('0') << std::setw(6) << micros.count();
#else
    localtime_s(&tmBuf, &time);
    oss << std::put_time(&tmBuf, "%F %T") <<  "." << std::setfill('0') << std::setw(6) << micros.count();
#endif
    return oss.str();
}

/**
 * Get current local time as string.
 */
inline std::string getCurrentTime()
{
    return getLocalTimeString(std::chrono::system_clock::now());
}

/**
 * Get random string.
 */
inline std::string getRandomString()
{
    using namespace std::chrono;
    std::uint32_t timestamp = static_cast<std::uint32_t>(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());

    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<std::uint64_t> uniform_dist(0, 0xFFFFFFFF);
    std::uint64_t rand = uniform_dist(e);

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(sizeof(std::uint32_t) * 2) << std::hex << timestamp << "-" << rand;
    return oss.str();
}

/**
 * Get librdkafka version string.
 */
inline std::string getLibRdKafkaVersion()
{
    return rd_kafka_version_str();
}

/**
 * Current number of threads created by rdkafka.
 */
inline int getLibRdKafkaThreadCount()
{
    return rd_kafka_thread_cnt();
}

} // end of Utility

} // end of KAFKA_API

