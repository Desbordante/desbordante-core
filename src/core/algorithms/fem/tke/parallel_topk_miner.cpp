#include "core/algorithms/fem/tke/parallel_topk_miner.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/lockfree/queue.hpp>

namespace algos::tke {

namespace {

struct Task {
    ParallelEpisode* parent;
    model::Event start_event;
};

}  // namespace

ParallelTopKMiner::ParallelTopKMiner(model::Event events_num, size_t k,
                                     std::vector<std::shared_ptr<LocationList>> events_loc_lists,
                                     size_t threads_num)
    : events_num_(events_num),
      k_(k),
      events_loc_lists_(std::move(events_loc_lists)),
      threads_num_(threads_num) {}

bool ParallelTopKMiner::TryAdd(ParallelEpisode ep, TopK& top_k, Explore& explore) const {
    size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : 1;
    if (ep.GetSupport() < minsup) return false;

    if (top_k.size() < k_ || ep.GetSupport() > minsup) {
        explore.push(ep);
    }

    if (top_k.size() < k_) {
        top_k.push(std::move(ep));
        return top_k.size() == k_;
    } else if (ep.GetSupport() > minsup) {
        top_k.pop();
        top_k.push(std::move(ep));
        return true;
    }
    return false;
}

void ParallelTopKMiner::ExploreParallel(TopK& top_k, Explore& explore) const {
    boost::lockfree::queue<Task> tasks(10000);
    boost::lockfree::queue<ParallelEpisode*> processed(10000);

    std::atomic<size_t> atomic_minsup{(top_k.size() == k_) ? top_k.top().GetSupport() : 1};
    std::atomic<bool> terminate_flag{false};
    std::atomic<size_t> tasks_in_flight{0};
    boost::interprocess::interprocess_semaphore tasks_sem{0};

    auto worker_loop = [&]() {
        while (true) {
            tasks_sem.wait();
            if (terminate_flag.load(std::memory_order_relaxed)) break;

            Task task;
            tasks.pop(task);

            size_t const task_minsup = atomic_minsup.load(std::memory_order_relaxed);
            if (task.parent->GetSupport() < task_minsup) {
                delete task.parent;
                tasks_in_flight.fetch_sub(1, std::memory_order_release);
                continue;
            }

            for (model::Event event = task.start_event; event < events_num_; ++event) {
                ParallelEpisode child =
                        task.parent->ParallelExtension(event, *events_loc_lists_[event]);

                if (child.GetSupport() >= atomic_minsup.load(std::memory_order_relaxed)) {
                    auto* result = new ParallelEpisode(std::move(child));
                    while (!processed.push(result)) {
                    }
                }
            }

            delete task.parent;
            tasks_in_flight.fetch_sub(1, std::memory_order_release);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(threads_num_);
    for (size_t i = 0; i < threads_num_; ++i) {
        workers.emplace_back(worker_loop);
    }

    size_t const high_watermark = threads_num_ * 4;

    while (true) {
        ParallelEpisode* ep_ptr = nullptr;
        while (processed.pop(ep_ptr)) {
            std::unique_ptr<ParallelEpisode> ep(ep_ptr);
            if (TryAdd(std::move(*ep), top_k, explore)) {
                atomic_minsup.store(top_k.top().GetSupport(), std::memory_order_relaxed);
            }
        }

        while (tasks_in_flight.load(std::memory_order_relaxed) < high_watermark &&
               !explore.empty()) {
            ParallelEpisode parent_ep = std::move(const_cast<ParallelEpisode&>(explore.top()));
            explore.pop();

            size_t const minsup_now = atomic_minsup.load(std::memory_order_relaxed);
            bool const stale = (top_k.size() == k_) ? parent_ep.GetSupport() <= minsup_now
                                                    : parent_ep.GetSupport() < minsup_now;
            if (stale) continue;

            model::Event const start_event = parent_ep.GetLastEvent() + 1;
            if (start_event >= events_num_) continue;

            auto* parent = new ParallelEpisode(std::move(parent_ep));
            tasks_in_flight.fetch_add(1, std::memory_order_relaxed);
            if (!tasks.push({parent, start_event})) {
                delete parent;
                tasks_in_flight.fetch_sub(1, std::memory_order_release);
                break;
            }
            tasks_sem.post();
        }

        if (explore.empty() && tasks_in_flight.load(std::memory_order_acquire) == 0) {
            bool got_any = false;
            while (processed.pop(ep_ptr)) {
                got_any = true;
                std::unique_ptr<ParallelEpisode> ep(ep_ptr);
                if (TryAdd(std::move(*ep), top_k, explore)) {
                    atomic_minsup.store(top_k.top().GetSupport(), std::memory_order_relaxed);
                }
            }
            if (got_any) continue;
            break;
        }
    }

    terminate_flag.store(true, std::memory_order_relaxed);
    for (size_t i = 0; i < threads_num_; ++i) tasks_sem.post();
    for (std::thread& t : workers) t.join();
}

void ParallelTopKMiner::ExploreSequential(TopK& top_k, Explore& explore) const {
    while (!explore.empty()) {
        ParallelEpisode item = std::move(const_cast<ParallelEpisode&>(explore.top()));
        explore.pop();

        size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : 1;
        bool const stale =
                (top_k.size() == k_) ? item.GetSupport() <= minsup : item.GetSupport() < minsup;
        if (stale) {
            explore = Explore();
            break;
        }

        for (model::Event event = item.GetLastEvent() + 1; event < events_num_; ++event) {
            TryAdd(item.ParallelExtension(event, *events_loc_lists_[event]), top_k, explore);
        }
    }
}

std::vector<ParallelEpisode> ParallelTopKMiner::Mine() {
    TopK top_k;
    Explore explore;

    for (ParallelEpisode& seed :
         ParallelEpisode::BuildParallelEpisodesWithEvents(events_loc_lists_, events_num_)) {
        TryAdd(std::move(seed), top_k, explore);
    }

    if (threads_num_ > 1) {
        ExploreParallel(top_k, explore);
    } else {
        ExploreSequential(top_k, explore);
    }

    std::vector<ParallelEpisode> result(top_k.size());
    int index = top_k.size() - 1;
    while (!top_k.empty()) {
        result[index] = std::move(const_cast<ParallelEpisode&>(top_k.top()));
        top_k.pop();
        index--;
    }
    return result;
}

}  // namespace algos::tke
