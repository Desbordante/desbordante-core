#include "core/algorithms/fem/tke/parallel_topk_miner.h"

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace algos::tke {

namespace {

struct ParentContext {
    ParallelEpisode episode;
    std::atomic<int> ref_count;

    ParentContext(ParallelEpisode&& ep, int valid_ext_count)
        : episode(std::move(ep)), ref_count(valid_ext_count) {}
};

struct Task {
    ParentContext* parent;
    model::Event event;
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

    auto worker_loop = [&]() {
        Task task;
        while (!terminate_flag.load(std::memory_order_relaxed)) {
            if (tasks.pop(task)) {
                size_t const cur_min = atomic_minsup.load(std::memory_order_relaxed);

                if (task.parent->episode.GetSupport() >= cur_min) {
                    ParallelEpisode child = task.parent->episode.ParallelExtension(
                            task.event, *events_loc_lists_[task.event]);

                    if (child.GetSupport() >= atomic_minsup.load(std::memory_order_relaxed)) {
                        processed.push(new ParallelEpisode(std::move(child)));
                    }
                }

                if (task.parent->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    delete task.parent;
                }
                tasks_in_flight.fetch_sub(1, std::memory_order_release);
            } else {
                std::this_thread::yield();
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(threads_num_);
    for (size_t i = 0; i < threads_num_; ++i) {
        workers.emplace_back(worker_loop);
    }

    size_t const high_watermark = threads_num_ * 4;
    ParentContext* current_parent = nullptr;
    model::Event current_event = 0;
    model::Event end_event = 0;

    while (true) {
        bool did_work = false;

        ParallelEpisode* ep_ptr = nullptr;
        while (processed.pop(ep_ptr)) {
            did_work = true;
            std::unique_ptr<ParallelEpisode> ep(ep_ptr);
            if (TryAdd(std::move(*ep), top_k, explore)) {
                atomic_minsup.store(top_k.top().GetSupport(), std::memory_order_relaxed);
            }
        }

        size_t const cur_min = atomic_minsup.load(std::memory_order_relaxed);
        if (!explore.empty() && explore.top().GetSupport() < cur_min) {
            explore = Explore();
            did_work = true;
        }

        while (tasks_in_flight.load(std::memory_order_relaxed) < high_watermark) {
            if (current_parent == nullptr) {
                if (explore.empty()) break;

                ParallelEpisode parent_ep = std::move(const_cast<ParallelEpisode&>(explore.top()));
                explore.pop();

                size_t const minsup_now = atomic_minsup.load(std::memory_order_relaxed);
                bool const stale = (top_k.size() == k_) ? parent_ep.GetSupport() <= minsup_now
                                                         : parent_ep.GetSupport() < minsup_now;
                if (stale) continue;

                model::Event start_event = parent_ep.GetLastEvent() + 1;
                if (start_event >= events_num_) continue;

                size_t valid_n = events_num_ - start_event;
                current_parent = new ParentContext(std::move(parent_ep), static_cast<int>(valid_n));
                current_event = start_event;
                end_event = events_num_;
            }

            tasks_in_flight.fetch_add(1, std::memory_order_relaxed);
            if (!tasks.push({current_parent, current_event})) {
                tasks_in_flight.fetch_sub(1, std::memory_order_release);
                break;
            }
            did_work = true;
            ++current_event;

            if (current_event == end_event) {
                current_parent = nullptr;
            }
        }

        if (explore.empty() && current_parent == nullptr &&
            tasks_in_flight.load(std::memory_order_acquire) == 0) {
            if (processed.pop(ep_ptr)) {
                std::unique_ptr<ParallelEpisode> ep(ep_ptr);
                TryAdd(std::move(*ep), top_k, explore);
                continue;
            }
            break;
        }

        if (!did_work) {
            std::this_thread::yield();
        }
    }

    terminate_flag.store(true, std::memory_order_relaxed);
    for (std::thread& t : workers) t.join();
}

void ParallelTopKMiner::ExploreSequential(TopK& top_k, Explore& explore) const {
    while (!explore.empty()) {
        ParallelEpisode item = std::move(const_cast<ParallelEpisode&>(explore.top()));
        explore.pop();

        size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : 1;
        bool const stale = (top_k.size() == k_) ? item.GetSupport() <= minsup
                                                 : item.GetSupport() < minsup;
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
