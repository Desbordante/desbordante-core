#include "logging.h"

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
// A global flag set by the Python atexit handler to signal that the
// interpreter is shutting down. The logging sink checks this flag to prevent
// unsafe calls back into the Python C-API during this critical phase.
bool g_is_shutting_down = false;

constexpr int kPyTraceLevel = 5;

template <typename Mutex>
class PythonLoggerSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit PythonLoggerSink(py::object py_logger) : py_logger_(std::move(py_logger)) {}

protected:
    void sink_it_(spdlog::details::log_msg const& msg) override {
        py::gil_scoped_acquire gil;

        if (g_is_shutting_down) {
            return;
        }

        int msg_py_level;
        switch (msg.level) {
            case spdlog::level::trace:
                msg_py_level = kPyTraceLevel;
                break;
            case spdlog::level::debug:
                msg_py_level = 10;
                break;
            case spdlog::level::info:
                msg_py_level = 20;
                break;
            case spdlog::level::warn:
                msg_py_level = 30;
                break;
            case spdlog::level::err:
                msg_py_level = 40;
                break;
            case spdlog::level::critical:
                msg_py_level = 50;
                break;
            default:
                msg_py_level = 0;
                break;
        }

        if (!py_logger_.attr("isEnabledFor")(msg_py_level).template cast<bool>()) {
            return;
        }

        std::string_view payload(msg.payload.data(), msg.payload.size());
        (void)py_logger_.attr("log")(msg_py_level, payload);
    }

    void flush_() override {
        py::gil_scoped_acquire gil;
        if (g_is_shutting_down) {
            return;
        }
        for (auto handler : py_logger_.attr("handlers")) {
            if (py::hasattr(handler, "flush")) {
                handler.attr("flush")();
            }
        }
    }

private:
    py::object py_logger_;
};

using PythonSink = PythonLoggerSink<std::mutex>;

void SetupLoggingBridge() {
    py::gil_scoped_acquire gil;
    try {
        py::module_ logging = py::module_::import("logging");

        if (!py::hasattr(logging, "TRACE")) {
            logging.attr("addLevelName")(kPyTraceLevel, "TRACE");
            logging.add_object("TRACE", py::int_(kPyTraceLevel));
        }

        py::object py_logger = logging.attr("getLogger")("desbordante");
        if (py::len(py_logger.attr("handlers")) == 0) {
            py::object handler = logging.attr("NullHandler")();
            py_logger.attr("addHandler")(handler);
        }

        auto python_sink = std::make_shared<PythonSink>(std::move(py_logger));
        ::util::logging::EnsureInitialized("desbordante", {std::move(python_sink)});

    } catch (py::error_already_set const& e) {
        py::print("ERROR: Error during Python logging setup:", e.what());
    }
}

void CleanupAtExit() {
    g_is_shutting_down = true;
    spdlog::shutdown();
}

}  // namespace

namespace python_bindings {

void BindLogging(py::module_&) {
    static std::once_flag once;
    std::call_once(once, SetupLoggingBridge);

    py::module_::import("atexit").attr("register")(py::cpp_function(CleanupAtExit));
}

}  // namespace python_bindings
