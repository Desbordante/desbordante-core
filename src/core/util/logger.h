#pragma once

#include <memory>
#include <string>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace util::logging {

inline void EnsureInitialized(std::string const& logger_name = "desbordante",
                              std::vector<spdlog::sink_ptr> sinks = {}) {
    if (spdlog::get(logger_name)) {
        return;
    }

    if (sinks.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    }

    auto logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

    logger->set_level(spdlog::level::debug);

    spdlog::register_logger(logger);
}

inline std::shared_ptr<spdlog::logger> GetLogger(std::string const& logger_name = "desbordante") {
    std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
    if (!logger) {
        EnsureInitialized(logger_name);
        logger = spdlog::get(logger_name);
        assert(logger);
    }
    return logger;
}

inline std::shared_ptr<spdlog::logger>& GetCachedLogger() {
    static thread_local std::shared_ptr<spdlog::logger> s_logger = ::util::logging::GetLogger();
    return s_logger;
}
}  // namespace util::logging

#if !defined(SPDLOG_ACTIVE_LEVEL)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::util::logging::GetCachedLogger(), __VA_ARGS__)
#else
#define LOG_TRACE(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::util::logging::GetCachedLogger(), __VA_ARGS__)
#else
#define LOG_DEBUG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(::util::logging::GetCachedLogger(), __VA_ARGS__)
#else
#define LOG_INFO(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(::util::logging::GetCachedLogger(), __VA_ARGS__)
#else
#define LOG_WARN(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::util::logging::GetCachedLogger(), __VA_ARGS__)
#else
#define LOG_ERROR(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::util::logging::GetCachedLogger(), __VA_ARGS__)
#else
#define LOG_CRITICAL(...) (void)0
#endif
