#include "composite_episode_miner.h"

#include <thread>
#include <boost/asio/post.hpp>

namespace algos::maxfem {

CompositeEpisodeMiner::CompositeEpisodeMiner(size_t min_support, size_t window_length)
    : min_support_(min_support), window_length_(window_length) {}

CompositeEpisodeMiner::Context::Context(boost::asio::thread_pool& p, 
                                        std::vector<ParallelEpisode> const& seeds,
                                        size_t min, size_t win)
    : pool(p), all_seeds(seeds), min_support(min), window_length(win) {
    max_parallel_tasks = std::thread::hardware_concurrency() * 4;
}

void CompositeEpisodeMiner::Context::Commit(MaxEpisodesCollection&& local_buf) {
    std::lock_guard<std::mutex> lock(results_mutex);
    global_results.push_back(std::move(local_buf));
}

std::vector<MaxEpisodesCollection> CompositeEpisodeMiner::Mine(const std::vector<ParallelEpisode>& seeds) {
    boost::asio::thread_pool pool(std::thread::hardware_concurrency());
    Context ctx(pool, seeds, min_support_, window_length_);

    for (const auto& seed : seeds) {
        ctx.tasks_in_flight++;
        boost::asio::post(pool, [this, seed, &ctx]() {
            BoundList bound_list(seed);
            CompositeEpisode episode({seed.GetEventSetPtr()}, seed.GetSupport());
            RunSearchTask(std::move(episode), std::move(bound_list), ctx);
            ctx.tasks_in_flight--;
        });
    }

    pool.join();
    return std::move(ctx.global_results);
}

void CompositeEpisodeMiner::RunSearchTask(CompositeEpisode episode, BoundList bound_list, Context& ctx) {
    MaxEpisodesCollection local_buffer;

    auto recurse = [&](auto&& self, CompositeEpisode& curr_ep, const BoundList& curr_bl) -> void {
        bool is_maximal = true;

        for (const auto& seed : ctx.all_seeds) {
            auto extended_bl = curr_bl.Extend(seed.GetLocationList(), ctx.min_support, ctx.window_length);

            if (extended_bl) {
                is_maximal = false;
                CompositeEpisode child = curr_ep;
                child.Extend(seed, extended_bl->GetSupport());

                int current_tasks = ctx.tasks_in_flight.load(std::memory_order_relaxed);
                if (current_tasks < ctx.max_parallel_tasks) {
                    ctx.tasks_in_flight++;
                    boost::asio::post(ctx.pool, [this, child = std::move(child), 
                                                 bl = std::move(*extended_bl), 
                                                 &ctx]() mutable {
                        RunSearchTask(std::move(child), std::move(bl), ctx);
                        ctx.tasks_in_flight--;
                    });
                } else {
                    self(self, child, *extended_bl);
                }
            }
        }

        if (is_maximal) {
            local_buffer.SimpleAdd(curr_ep);
        }
    };

    recurse(recurse, episode, bound_list);
    ctx.Commit(std::move(local_buffer));
}

} // namespace algos::maxfem
