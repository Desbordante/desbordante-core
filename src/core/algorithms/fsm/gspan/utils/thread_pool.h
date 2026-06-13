#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace gspan {

class ThreadPool {
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::function<void(int)>> tasks_;
    std::vector<std::thread> threads_;
    bool stop_ = false;

    void WorkerLoop(int thread_id) {
        while (true) {
            std::function<void(int)> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.back());
                tasks_.pop_back();
            }
            task(thread_id);
        }
    }

public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this, i] { WorkerLoop(i); });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : threads_) t.join();
    }

    template <typename F>
    void Spawn(F&& f, std::atomic<int>& pending_counter) {
        pending_counter.fetch_add(1, std::memory_order_relaxed);
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace_back([f = std::forward<F>(f), &pending_counter](int thread_id) {
                f(thread_id);
                pending_counter.fetch_sub(1, std::memory_order_release);
            });
        }
        cv_.notify_one();
    }

    void Wait(std::atomic<int>& pending_counter, int current_thread_id) {
        while (pending_counter.load(std::memory_order_acquire) > 0) {
            std::function<void(int)> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (!tasks_.empty()) {
                    task = std::move(tasks_.back());
                    tasks_.pop_back();
                }
            }
            if (task) {
                task(current_thread_id);
            } else {
                std::this_thread::yield();
            }
        }
    }
};

}  // namespace gspan
