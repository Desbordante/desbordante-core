#include "py_util/logging.h"

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#ifdef PYBIND11_HAS_SUBINTERPRETER_SUPPORT
#include <pybind11/subinterpreter.h>
#endif

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include "util/logger.h"

namespace {
namespace py = pybind11;

constexpr int kPyTraceLevel = 5;
using InterpreterId = int64_t;

InterpreterId GetCurrentInterpreterId() {
#ifdef PYBIND11_HAS_SUBINTERPRETER_SUPPORT
    return py::subinterpreter::current().id();
#else
    return 0;
#endif
}

template <typename Mutex>
class MultiTenantPythonSink : public spdlog::sinks::base_sink<Mutex> {
public:
    MultiTenantPythonSink() = default;

    void RegisterLogger(py::object logger) {
        InterpreterId id = GetCurrentInterpreterId();
        if (id < 0) return;

        std::unique_lock lock(registry_mutex_);
        interpreters_loggers_.try_emplace(id, std::move(logger));
    }

    void UnregisterLogger() {
        InterpreterId id = GetCurrentInterpreterId();
        if (id < 0) return;

        std::unique_lock lock(registry_mutex_);
        interpreters_loggers_.erase(id);
    }

protected:
    void sink_it_(spdlog::details::log_msg const& msg) override {
        py::gil_scoped_acquire gil;

        py::object target_logger = GetCurrentSubinterpreterLogger();
        if (!target_logger) {
            return;
        }

        static std::unordered_map<spdlog::level::level_enum, int> const kLevelMap{
                {spdlog::level::trace, kPyTraceLevel},
                {spdlog::level::debug, 10},
                {spdlog::level::info, 20},
                {spdlog::level::warn, 30},
                {spdlog::level::err, 40},
                {spdlog::level::critical, 50},
                {spdlog::level::off, 0}};

        int msg_py_level = 0;
        auto map_it = kLevelMap.find(msg.level);
        if (map_it != kLevelMap.end()) {
            msg_py_level = map_it->second;
        }

        if (!ShouldLog(target_logger, msg_py_level)) {
            return;
        }
        std::string_view payload(msg.payload.data(), msg.payload.size());
        (void)target_logger.attr("log")(msg_py_level, payload);
    }

    void flush_() override {}

private:
    py::object GetCurrentSubinterpreterLogger() {
        InterpreterId id = GetCurrentInterpreterId();
        if (id < 0) return {};

        std::shared_lock lock(registry_mutex_);
        auto it = interpreters_loggers_.find(id);
        if (it == interpreters_loggers_.end()) {
            return {};
        }
        return it->second;
    }

    bool ShouldLog(py::object const& logger, int level) const {
        return logger.attr("isEnabledFor")(level).cast<bool>();
    }

    std::shared_mutex registry_mutex_;
    std::unordered_map<InterpreterId, py::object> interpreters_loggers_;
};

using PythonSink = MultiTenantPythonSink<spdlog::details::null_mutex>;

std::shared_ptr<PythonSink> GetGlobalPythonSink() {
    static auto sink = std::make_shared<PythonSink>();
    return sink;
}

void SetupLoggingBridge() {
    py::gil_scoped_acquire gil;
    try {
        py::module_ logging = py::module_::import("logging");

        if (!py::hasattr(logging, "TRACE")) {
            logging.attr("addLevelName")(kPyTraceLevel, "TRACE");
            logging.add_object("TRACE", py::int_(kPyTraceLevel));
        }

        py::object py_logger = logging.attr("getLogger")("desbordante");

        auto python_sink = GetGlobalPythonSink();
        python_sink->RegisterLogger(std::move(py_logger));

        ::util::logging::EnsureInitialized("desbordante", {std::move(python_sink)});
    } catch (py::error_already_set const& e) {
        py::print("ERROR: Error during Python logging setup:", e.what(),
                  py::arg("file") = py::module_::import("sys").attr("stderr"));
        throw;
    }
}

void CleanupAtExit() {
    if (auto sink = GetGlobalPythonSink()) {
        sink->UnregisterLogger();
    }
}

}  // namespace

namespace python_bindings {

void BindLogging(py::module_&) {
    SetupLoggingBridge();
    py::module_::import("atexit").attr("register")(py::cpp_function(CleanupAtExit));
}

}  // namespace python_bindings
