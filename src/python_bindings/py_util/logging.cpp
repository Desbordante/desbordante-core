#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include "util/logger.h"

namespace {
namespace py = pybind11;

constexpr int kPyTraceLevel = 5;

py::object g_proxy_instance = py::none();
bool g_is_shutting_down = false;

template <typename Mutex>
class PythonLoggerSink : public spdlog::sinks::base_sink<Mutex> {
public:
    PythonLoggerSink() {
        py::gil_scoped_acquire gil;
        try {
            py_logger_ = py::module_::import("logging").attr("getLogger")("desbordante");
        } catch (py::error_already_set& e) {
            py::print("CRITICAL ERROR: Failed to get Python logger 'desbordante'.", e.what());
            py_logger_ = py::none();
        }
    }

protected:
    void sink_it_(spdlog::details::log_msg const& msg) override {
        if (g_is_shutting_down || py_logger_.is_none()) return;

        py::gil_scoped_acquire gil;
        int level;
        switch (msg.level) {
            case spdlog::level::trace:
                level = kPyTraceLevel;
                break;
            case spdlog::level::debug:
                level = 10;
                break;
            case spdlog::level::info:
                level = 20;
                break;
            case spdlog::level::warn:
                level = 30;
                break;
            case spdlog::level::err:
                level = 40;
                break;
            case spdlog::level::critical:
                level = 50;
                break;
            default:
                level = 0;
                break;
        }

        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        std::string_view payload(formatted.data(), formatted.size());

        if (!payload.empty() && payload.back() == '\n') {
            payload.remove_suffix(1);
        }

        (void)py_logger_.attr("log")(level, std::string(payload));
    }

    void flush_() override {}

private:
    py::object py_logger_;
};

using PythonSink = PythonLoggerSink<std::mutex>;

void SetCppLoggingLevel(spdlog::level::level_enum level) {
    if (auto const logger = ::util::logging::GetLogger()) {
        logger->set_level(level);
    }
}

spdlog::level::level_enum PyLevelToSpdlogLevel(int py_level) {
    if (py_level > 0 && py_level <= kPyTraceLevel) return spdlog::level::trace;
    if (py_level > 0 && py_level <= 10) return spdlog::level::debug;
    if (py_level > 0 && py_level <= 20) return spdlog::level::info;
    if (py_level > 0 && py_level <= 30) return spdlog::level::warn;
    if (py_level > 0 && py_level <= 40) return spdlog::level::err;
    if (py_level > 0) return spdlog::level::critical;
    return spdlog::level::off;
}

int SpdlogLevelToPyLevel(spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace:
            return kPyTraceLevel;
        case spdlog::level::debug:
            return 10;
        case spdlog::level::info:
            return 20;
        case spdlog::level::warn:
            return 30;
        case spdlog::level::err:
            return 40;
        case spdlog::level::critical:
            return 50;
        case spdlog::level::off:
            return 60;
        default:
            return 0;
    }
}

struct LoggerProxy {
    py::object real_logger;
};

void SetupLoggingBridge() {
    auto python_sink = std::make_shared<PythonSink>();
    ::util::logging::Initialize("desbordante", {python_sink});

    try {
        py::gil_scoped_acquire gil;
        py::module_ logging = py::module_::import("logging");
        py::object py_logger = logging.attr("getLogger")("desbordante");
        if (py::len(py_logger.attr("handlers")) == 0) {
            py::object handler = logging.attr("StreamHandler")();
            py::object formatter = logging.attr("Formatter")("%(message)s");
            (void)handler.attr("setFormatter")(formatter);
            (void)handler.attr("setLevel")(logging.attr("NOTSET"));
            (void)py_logger.attr("addHandler")(handler);
            (void)py_logger.attr("setLevel")(logging.attr("INFO"));
            py_logger.attr("propagate") = false;
        }

    } catch (py::error_already_set const& e) {
        py::print("ERROR: Error during Python logging setup:", e.what());
    }
}

void CleanupAtExit() {
    g_is_shutting_down = true;
    g_proxy_instance = py::none();
    spdlog::shutdown();
}

}  // namespace

namespace python_bindings {

void BindLogging(py::module_& main_module) {
    auto m = main_module.def_submodule("logging",
                                       "Logging configuration for the desbordante library.");

    static std::once_flag once;
    std::call_once(once, SetupLoggingBridge);

    py::module_::import("atexit").attr("register")(py::cpp_function(CleanupAtExit));

    py::class_<LoggerProxy>(
            m, "Logger",
            "A proxy to a standard Python logger that syncs its level with the C++ core.")
            .def(py::init([](py::object logger_obj) { return LoggerProxy{std::move(logger_obj)}; }),
                 py::arg("logger"))

            .def(
                    "setLevel",
                    [](LoggerProxy const& self, spdlog::level::level_enum level) {
                        (void)self.real_logger.attr("setLevel")(SpdlogLevelToPyLevel(level));
                        SetCppLoggingLevel(level);
                    },
                    py::arg("level"),
                    "Sets the logging level using the desbordante.logging.Level enum.")

            .def(
                    "setLevel",
                    [](LoggerProxy const& self, int level) {
                        (void)self.real_logger.attr("setLevel")(level);
                        SetCppLoggingLevel(PyLevelToSpdlogLevel(level));
                    },
                    py::arg("level"),
                    "Sets the logging level using a standard Python integer level (e.g., "
                    "logging.INFO).")

            .def("__getattr__", [](LoggerProxy const& self, std::string const& name) {
                return self.real_logger.attr(name.c_str());
            });

    m.def(
            "get_logger",
            []() -> py::object {
                if (g_is_shutting_down) {
                    py::print(
                            "Warning: get_logger() called during interpreter shutdown. Returning "
                            "None.");
                    return py::none();
                }
                if (g_proxy_instance.is_none()) {
                    py::gil_scoped_acquire gil;
                    py::object real_logger =
                            py::module_::import("logging").attr("getLogger")("desbordante");
                    auto proxy_class = py::module_::import("desbordante.logging").attr("Logger");
                    g_proxy_instance = proxy_class(real_logger);
                }
                return g_proxy_instance;
            },
            py::return_value_policy::reference,
            "Returns a proxy to the standard Python logger that syncs its level with the C++ "
            "core.");

    py::enum_<spdlog::level::level_enum>(m, "Level", "Logging severity level")
            .value("TRACE", spdlog::level::trace)
            .value("DEBUG", spdlog::level::debug)
            .value("INFO", spdlog::level::info)
            .value("WARNING", spdlog::level::warn)
            .value("ERROR", spdlog::level::err)
            .value("CRITICAL", spdlog::level::critical)
            .value("OFF", spdlog::level::off)
            .export_values();
}

}  // namespace python_bindings
