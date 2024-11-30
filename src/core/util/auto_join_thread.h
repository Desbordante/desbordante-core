#pragma once

#include <thread>

namespace util::jthread {

/// @brief Simple RAII wrapper for std::thread. Joins on destruction.
/// @remark The class is inspired by Scott Meyers' ThreadRAII (from Effective Modern C++)
class AutoJoinThread {
public:
    explicit AutoJoinThread(std::thread&& t) : t(std::move(t)) {}

    AutoJoinThread(AutoJoinThread&&) = default;
    AutoJoinThread& operator=(AutoJoinThread&&) = default;
    // std::thread is not copyable:
    AutoJoinThread(AutoJoinThread&) = delete;
    AutoJoinThread& operator=(AutoJoinThread&) = delete;

    template <typename F, typename... Args>
    explicit AutoJoinThread(F&& f, Args&&... args)
        : AutoJoinThread(std::thread{std::forward<F>(f), std::forward<Args>(args)...}) {}

    ~AutoJoinThread() {
        if (t.joinable()) {
            t.join();
        }
    }

    std::thread& get() {
        return t;
    }

private:
    std::thread t;
};

}  // namespace util::jthread

namespace util {

#ifdef __cpp_lib_jthread
using JThread = std::jthread;
#else
using JThread = jthread::AutoJoinThread;
#endif

}  // namespace util
