#include <cstddef>

#include <gtest/gtest.h>

#include "core/util/worker_thread_pool.h"

namespace tests {

// Repro for a destruction-order race in WorkerThreadPool that surfaces on
// Apple-Clang/libc++ as:
//   libc++abi: terminating due to uncaught exception of type
//   std::__1::system_error: mutex lock failed: Invalid argument
//
// The pool's std::mutex is declared after worker_threads_ in the class body,
// so it is destroyed BEFORE the worker JThreads are joined. If a worker is
// still inside its unique_lock dtor (unlocking the mutex) when ~WorkerThreadPool
// returns and member destruction begins, the worker touches a destroyed mutex.
// pthread_mutex_unlock on a destroyed mutex returns EINVAL on macOS, which
// libc++'s std::mutex translates into a system_error and aborts.
//
// On Linux/glibc this is silently tolerated, so the bug does not show up.
//
// The two test cases construct + destroy the pool with no work submitted,
// which maximizes the race window: workers go straight from spawn into
// cond_var.wait, and the dtor wakes them with no SetWork pairing first.
TEST(WorkerThreadPool, ConstructAndDestroyWithoutWork) {
    util::WorkerThreadPool pool{4};
}

TEST(WorkerThreadPool, RepeatedConstructAndDestroyWithoutWork) {
    constexpr std::size_t iterations = 100;
    for (std::size_t i = 0; i < iterations; ++i) {
        util::WorkerThreadPool pool{4};
    }
}

}  // namespace tests
