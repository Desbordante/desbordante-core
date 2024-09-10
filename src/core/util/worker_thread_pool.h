#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>
#include <vector>

#include "model/index.h"
#include "util/barrier.h"
#include "util/desbordante_assume.h"

namespace util {
class WorkerThreadPool {
public:
    // Must be thread-safe.
    using Worker = std::function<void()>;

private:
    struct Completion {
        WorkerThreadPool& pool;

        void operator()() {
            pool.IsWorkingAction([](bool& is_working) { is_working = false; });
        }
    };

    struct DoWork {
        void operator()(WorkerThreadPool& pool) {
            DESBORDANTE_ASSUME(pool.work_);
            try {
                pool.work_();
            } catch (...) {
                pool.barrier_.ArriveAndWait();
                throw;
            }
            pool.barrier_.ArriveAndWait();
        }
    };

    Worker work_;
    std::vector<std::jthread> worker_threads_;
    std::vector<std::packaged_task<void(WorkerThreadPool&)>> tasks_;
    util::Barrier<Completion> barrier_;
    std::condition_variable working_var_;
    std::mutex working_mutex_;
    bool is_working_ = false;

    void IsWorkingAction(auto action) {
        std::unique_lock<std::mutex> lk{working_mutex_};
        action(is_working_);
    }

    void Work(std::packaged_task<void(WorkerThreadPool&)>& thread_task) {
        while (true) {
            {
                std::unique_lock<std::mutex> lk{working_mutex_};
                working_var_.wait(lk, [this]() { return is_working_; });
            }
            if (!work_) break;
            ResetAndWork(thread_task);
        }
    }

    void Terminate() {
        SetWork(nullptr);
    }

    void ResetAndWork(std::packaged_task<void(WorkerThreadPool&)>& thread_task) {
        thread_task.reset();
        thread_task(*this);
    }

public:
    WorkerThreadPool(std::size_t thread_num);

    // Previous tasks must be finished before calling this.
    template <typename WorkerType>
    void SetWork(WorkerType work) {
        work_ = std::move(work);
        IsWorkingAction([](bool& is_working) { is_working = true; });
        working_var_.notify_all();
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
        Wait();
    }

    void ExecIndexWithResource(auto do_work, auto acquire_resource, model::Index size) {
        ExecIndexWithResource(std::move(do_work), std::move(acquire_resource), size,
                              [](auto&&...) {});
    }

    template <typename FunctionType>
    void ExecIndex(FunctionType func, model::Index size) {
        ExecIndexWithResource([func = std::move(func)](model::Index i, auto) { func(i); },
                              []() { return std::monostate{}; }, size, [](auto&&...) {});
    }

    // Main thread must call this to finish.
    void Wait() {
        std::packaged_task<void(WorkerThreadPool&)>& main_task = tasks_.front();
        ResetAndWork(main_task);
        // Rethrow exceptions
        for (std::packaged_task<void(WorkerThreadPool&)>& task : tasks_) {
            task.get_future().get();
        }
    }

    std::size_t ThreadNum() const noexcept {
        return worker_threads_.size() + 1;
    }

    ~WorkerThreadPool() {
        Terminate();
    }
};
}  // namespace util
