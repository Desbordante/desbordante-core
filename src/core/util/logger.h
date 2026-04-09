#pragma once

#include <memory>
#include <string>
#include <vector>

// IWYU pragma: begin_exports
#include <spdlog/fmt/bundled/base.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
// IWYU pragma: end_exports

#include "core/util/export.h"

namespace util::logging {

inline DESBORDANTE_EXPORT std::shared_ptr<spdlog::logger>& Initialize(
        std::vector<spdlog::sink_ptr> sinks = {}) {
    static std::shared_ptr<spdlog::logger> core_logger = [&sinks]() {
        if (sinks.empty()) {
            sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
        }

        // We don't really care about the name, since we don't use the registry.
        auto logger = std::make_shared<spdlog::logger>("desbordante", sinks.begin(), sinks.end());
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

        logger->set_level(spdlog::level::trace);

        return logger;
    }();

    return core_logger;
}
}  // namespace util::logging

// Ignore arguments, but mark them as "used"
#define DESBORDANTE_NOOP(...)                 \
    do {                                      \
        (void)sizeof((void)(__VA_ARGS__), 0); \
    } while (false)

#if !defined(SPDLOG_ACTIVE_LEVEL)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::util::logging::Initialize(), __VA_ARGS__)
#else
#define LOG_TRACE(...) DESBORDANTE_NOOP(__VA_ARGS__)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::util::logging::Initialize(), __VA_ARGS__)
#else
#define LOG_DEBUG(...) DESBORDANTE_NOOP(__VA_ARGS__)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(::util::logging::Initialize(), __VA_ARGS__)
#else
#define LOG_INFO(...) DESBORDANTE_NOOP(__VA_ARGS__)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(::util::logging::Initialize(), __VA_ARGS__)
#else
#define LOG_WARN(...) DESBORDANTE_NOOP(__VA_ARGS__)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::util::logging::Initialize(), __VA_ARGS__)
#else
#define LOG_ERROR(...) DESBORDANTE_NOOP(__VA_ARGS__)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::util::logging::Initialize(), __VA_ARGS__)
#else
#define LOG_CRITICAL(...) DESBORDANTE_NOOP(__VA_ARGS__)
#endif
