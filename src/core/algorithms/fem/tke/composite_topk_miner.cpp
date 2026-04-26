#include "core/algorithms/fem/tke/composite_topk_miner.h"

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace algos::tke {

namespace {

struct ParentContext {
    CompositeEpisode episode;
    std::atomic<int> ref_count;

    ParentContext(CompositeEpisode&& ep, int valid_ext_count)
        : episode(std::move(ep)), ref_count(valid_ext_count) {}
};

struct Task {
    ParentContext* parent;
    size_t ext_idx;
};

}  // namespace

CompositeTopKMiner::CompositeTopKMiner(size_t k, size_t window_length, size_t threads_num)
    : k_(k), window_length_(window_length), threads_num_(threads_num) {}

bool CompositeTopKMiner::TryAdd(CompositeEpisode ep, TopK& top_k, Explore& explore,
                                size_t floor_minsup) const {
    size_t const current_minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : floor_minsup;
    if (ep.GetSupport() < current_minsup) return false;

    if (top_k.size() < k_ || ep.GetSupport() > current_minsup) {
        explore.push(ep);
    }

    if (top_k.size() < k_) {
        top_k.push(std::move(ep));
        return top_k.size() == k_;
    } else if (ep.GetSupport() > top_k.top().GetSupport()) {
        top_k.pop();
        top_k.push(std::move(ep));
        return true;
    }
    return false;
}

void CompositeTopKMiner::ExploreParallel(std::vector<ParallelEpisode> const& parallel_episodes,
                                         TopK& top_k, Explore& explore,
                                         size_t initial_minsup) const {
    size_t const n = parallel_episodes.size();
    if (n == 0) return;

    boost::lockfree::queue<Task> tasks(10000);
    boost::lockfree::queue<CompositeEpisode*> processed(10000);

    std::atomic<size_t> atomic_minsup{(top_k.size() == k_) ? top_k.top().GetSupport()
                                                             : initial_minsup};
    std::atomic<bool> terminate_flag{false};
    std::atomic<size_t> tasks_in_flight{0};

    auto worker_loop = [&]() {
        Task task;
        while (!terminate_flag.load(std::memory_order_relaxed)) {
            if (tasks.pop(task)) {
                size_t const cur_min = atomic_minsup.load(std::memory_order_relaxed);
                ParallelEpisode const& ext = parallel_episodes[task.ext_idx];

                if (task.parent->episode.GetSupport() >= cur_min && ext.GetSupport() >= cur_min) {
                    std::optional<CompositeEpisode> child =
                            task.parent->episode.TryExtend(ext, cur_min, window_length_);
                    if (child &&
                        child->GetSupport() >= atomic_minsup.load(std::memory_order_relaxed)) {
                        processed.push(new CompositeEpisode(std::move(*child)));
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
    size_t current_ext_idx = 0;
    size_t current_ext_count = 0;

    while (true) {
        bool did_work = false;

        CompositeEpisode* ep_ptr = nullptr;
        while (processed.pop(ep_ptr)) {
            did_work = true;
            std::unique_ptr<CompositeEpisode> ep(ep_ptr);
            if (TryAdd(std::move(*ep), top_k, explore, initial_minsup)) {
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

                CompositeEpisode parent_ep =
                        std::move(const_cast<CompositeEpisode&>(explore.top()));
                explore.pop();

                size_t const minsup_now = atomic_minsup.load(std::memory_order_relaxed);
                bool const stale = (top_k.size() == k_) ? parent_ep.GetSupport() <= minsup_now
                                                         : parent_ep.GetSupport() < minsup_now;
                if (stale) continue;

                size_t valid_n = 0;
                while (valid_n < n && parallel_episodes[valid_n].GetSupport() >= minsup_now) {
                    ++valid_n;
                }
                if (valid_n == 0) continue;

                current_parent =
                        new ParentContext(std::move(parent_ep), static_cast<int>(valid_n));
                current_ext_idx = 0;
                current_ext_count = valid_n;
            }

            tasks_in_flight.fetch_add(1, std::memory_order_relaxed);
            if (!tasks.push({current_parent, current_ext_idx})) {
                tasks_in_flight.fetch_sub(1, std::memory_order_release);
                break;
            }
            did_work = true;
            ++current_ext_idx;

            if (current_ext_idx == current_ext_count) {
                current_parent = nullptr;
            }
        }

        if (explore.empty() && current_parent == nullptr &&
            tasks_in_flight.load(std::memory_order_acquire) == 0) {
            if (processed.pop(ep_ptr)) {
                std::unique_ptr<CompositeEpisode> ep(ep_ptr);
                TryAdd(std::move(*ep), top_k, explore, initial_minsup);
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

void CompositeTopKMiner::ExploreSequential(std::vector<ParallelEpisode> const& parallel_episodes,
                                           TopK& top_k, Explore& explore,
                                           size_t initial_minsup) const {
    while (!explore.empty()) {
        CompositeEpisode item = std::move(const_cast<CompositeEpisode&>(explore.top()));
        explore.pop();

        size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : initial_minsup;
        bool const stale = (top_k.size() == k_) ? item.GetSupport() <= minsup
                                                 : item.GetSupport() < minsup;
        if (stale) {
            explore = Explore();
            break;
        }

        for (ParallelEpisode const& ext : parallel_episodes) {
            if (ext.GetSupport() < minsup) break;
            std::optional<CompositeEpisode> child = item.TryExtend(ext, minsup, window_length_);
            if (child) {
                TryAdd(std::move(*child), top_k, explore, initial_minsup);
            }
        }
    }
}

std::vector<CompositeEpisode> CompositeTopKMiner::Mine(
        std::vector<ParallelEpisode>&& parallel_episodes, size_t initial_minsup) {
    if (parallel_episodes.empty()) {
        return {};
    }

    TopK top_k;
    Explore explore;

    for (ParallelEpisode const& p : parallel_episodes) {
        TryAdd(CompositeEpisode(p), top_k, explore, initial_minsup);
    }

    if (threads_num_ > 1) {
        ExploreParallel(parallel_episodes, top_k, explore, initial_minsup);
    } else {
        ExploreSequential(parallel_episodes, top_k, explore, initial_minsup);
    }

    std::vector<CompositeEpisode> result;
    result.reserve(top_k.size());
    while (!top_k.empty()) {
        result.push_back(std::move(const_cast<CompositeEpisode&>(top_k.top())));
        top_k.pop();
    }
    return result;
}

}  // namespace algos::tke
