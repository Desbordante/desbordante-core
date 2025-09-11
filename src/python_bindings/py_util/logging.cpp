#include "logging.h"

#include <unordered_map>

#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include "logger.h"

namespace {
namespace py = pybind11;
} // namespace

namespace python_bindings {
void BindLogging(py::module_& main_module) {
    util::logging::Initialize();

    auto logging_module = main_module.def_submodule("logging");

    auto set_log_level = [](std::string const& level) {
        static std::unordered_map<std::string, spdlog::level::level_enum> const kLevels = {
            {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
            {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
            {"error", spdlog::level::err},   {"off", spdlog::level::off}};

        auto logger = util::logging::GetLogger();
        if (!logger) {
            throw std::runtime_error("Logger 'desbordante' not initialized.");
        }

        if (auto const it = kLevels.find(level); it != kLevels.end()) {
            logger->set_level(it->second);
        } else {
            throw py::value_error(
                    "Invalid log level. Available options: "
                    "'trace', 'debug', 'info', 'warn', 'error', 'off'");
        }
    };

    logging_module.def("set_level", set_log_level, py::arg("level"),
                       R"(Set the logging verbosity level for the desbordante module.)");

    main_module.attr("logging") = logging_module;
}

} // namespace python_bindings

