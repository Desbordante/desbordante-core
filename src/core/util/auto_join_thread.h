#pragma once

#include <stdexcept>
#include <thread>

#include <easylogging++.h>

namespace util::jthread {

/// @brief Simple RAII wrapper for std::thread. Joins on destruction.
/// @remark The class is inspired by Scott Meyers' ThreadRAII (from Effective Modern C++)
class AutoJoinThread {
public:
    explicit AutoJoinThread(std::thread&& t) : t_(std::move(t)) {}

    AutoJoinThread(AutoJoinThread&&) = default;
    AutoJoinThread& operator=(AutoJoinThread&&) = default;
    // std::thread is not copyable:
    AutoJoinThread(AutoJoinThread&) = delete;
    AutoJoinThread& operator=(AutoJoinThread&) = delete;

    template <typename F, typename... Args>
    explicit AutoJoinThread(F&& f, Args&&... args)
        : AutoJoinThread(std::thread{std::forward<F>(f), std::forward<Args>(args)...}) {}

    ~AutoJoinThread() try {
        if (t_.joinable()) {
            t_.join();
        }
    } catch (std::system_error const& e) {
        LOG(ERROR) << e.what();
        return;  // Don't pass exception on
    }

    std::thread& Get() {
        return t_;
    }

private:
    std::thread t_;
};

}  // namespace util::jthread

namespace util {

#ifdef __cpp_lib_jthread
using JThread = std::jthread;
#else
using JThread = jthread::AutoJoinThread;
#endif

}  // namespace util
