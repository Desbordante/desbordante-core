#pragma once

#include <atomic>
#include <barrier>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>
#include <vector>

#include "model/index.h"

namespace util {
class WorkerThreadPool {
public:
    // Both functions must be thread-safe. Worker must return false when no work is left.
    using Worker = std::function<void()>;

private:
    struct Completion {
        WorkerThreadPool& thread_pool;

        void operator()() noexcept {
            thread_pool.is_working_.clear();
        }
    };

    Worker work_;
    std::vector<std::jthread> worker_threads_;
    std::barrier<Completion> barrier_;
    std::atomic_flag is_working_;

    void Work() {
        while (true) {
            is_working_.wait(false);
            if (!work_) break;
            WorkUntilComplete();
        }
    }

    void Terminate() {
        SetWork(nullptr);
    }

public:
    WorkerThreadPool(std::size_t thread_num);

    // Previous tasks must be finished before calling this.
    template <typename WorkerType>
    void SetWork(WorkerType work) {
        work_ = std::move(work);
        is_working_.test_and_set();
        is_working_.notify_all();
    }

    template <typename FunctionType>
    void ExecSingle(FunctionType func) {
        SetWork([func, flag = std::make_shared<std::once_flag>()]() {
            std::call_once(*flag, func);
        });
    }

    void ExecIndexWithResource(auto do_work, auto acquire_resource, model::Index size,
                               auto finish) {
        DESBORDANTE_ASSUME(size + ThreadNum() <= std::size_t{} - 1);
        std::atomic<model::Index> index = 0;
        auto work = [do_work = std::move(do_work), acquire_resource = std::move(acquire_resource),
                     size, finish = std::move(finish), &index]() {
            model::Index i;
            auto resource = acquire_resource();
            while ((i = index.fetch_add(1, std::memory_order::acquire)) < size) {
                do_work(i, resource);
            }
            finish(std::move(resource));
        };
        SetWork(work);
        WorkUntilComplete();
    }

    template <typename FunctionType>
    void ExecIndex(FunctionType func, model::Index size) {
        ExecIndexWithResource([func = std::move(func)](model::Index i, auto) { func(i); },
                              []() { return std::monostate{}; }, size, [](auto&&...) {});
    }

    // Main thread must call this to finish.
    void WorkUntilComplete() {
        auto call_unchecked = [](Worker const& w) {
            if (!w) __builtin_unreachable();
            w();
        };
        call_unchecked(work_);
        barrier_.arrive_and_wait();
    }

    ~WorkerThreadPool() {
        Terminate();
    }
};
}  // namespace util
