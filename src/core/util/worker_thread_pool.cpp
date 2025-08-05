#include "util/worker_thread_pool.h"

#include <cassert>

namespace util {
WorkerThreadPool::WorkerThreadPool(std::size_t thread_num)
    : barrier_(thread_num, Completion{*this}) {
    assert(thread_num > 1);
    tasks_.reserve(thread_num);
    tasks_.emplace_back(DoWork{});  // main thread task
    worker_threads_.reserve(--thread_num);
    try {
        while (thread_num--) {
            worker_threads_.emplace_back(&WorkerThreadPool::Work, this,
                                         std::ref(tasks_.emplace_back(DoWork{})));
        }
    } catch (std::system_error&) {
        Terminate();
        barrier_.Wait(barrier_.Arrive(++thread_num));
        throw;
    }
}
}  // namespace util
