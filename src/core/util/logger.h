#pragma once

#include <memory>
#include <string>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace util::logging {

inline void Initialize(std::string const& logger_name = "desbordante",
                       std::vector<spdlog::sink_ptr> sinks = {}) {
    if (spdlog::get(logger_name)) {
        return;
    }

    if (sinks.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    }

    auto logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

#ifndef NDEBUG
    logger->set_level(spdlog::level::debug);
#else
    logger->set_level(spdlog::level::info);
#endif

    spdlog::register_logger(logger);
}

inline std::shared_ptr<spdlog::logger> GetLogger(std::string const& logger_name = "desbordante") {
    return spdlog::get(logger_name);
}

}  // namespace util::logging

#define GET_CACHED_LOGGER()                                            \
    ([]() -> std::shared_ptr<spdlog::logger>& {                        \
        static thread_local std::shared_ptr<spdlog::logger> s_logger = \
                ::util::logging::GetLogger();                          \
        return s_logger;                                               \
    }())

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(GET_CACHED_LOGGER(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(GET_CACHED_LOGGER(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(GET_CACHED_LOGGER(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(GET_CACHED_LOGGER(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(GET_CACHED_LOGGER(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(GET_CACHED_LOGGER(), __VA_ARGS__)
