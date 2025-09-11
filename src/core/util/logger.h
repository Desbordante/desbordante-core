#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace util::logging {

inline void Initialize(std::string const& logger_name = "desbordante") {
    if (spdlog::get(logger_name)) {
        return;
    }
    auto const logger = spdlog::stderr_color_mt(logger_name);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
#ifndef NDEBUG
    logger->set_level(spdlog::level::debug);
#else
    logger->set_level(spdlog::level::off);
#endif
}

inline std::shared_ptr<spdlog::logger> GetLogger(std::string const& logger_name = "desbordante") {
    return spdlog::get(logger_name);
}

} // namespace util::logging

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::util::logging::GetLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::util::logging::GetLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(::util::logging::GetLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(::util::logging::GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::util::logging::GetLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::util::logging::GetLogger(), __VA_ARGS__)
