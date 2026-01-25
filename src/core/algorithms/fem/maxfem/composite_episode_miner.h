#pragma once

#include <atomic>
#include <mutex>
#include <vector>

#include <boost/asio/thread_pool.hpp>

#include "core/algorithms/fem/maxfem/model/bound_list.h"
#include "core/algorithms/fem/maxfem/model/composite_episode.h"
#include "core/algorithms/fem/maxfem/model/max_episodes_collection.h"
#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/config/thread_number/type.h"

namespace algos::maxfem {

class CompositeEpisodeMiner {
public:
    CompositeEpisodeMiner(size_t min_support, size_t window_length,
                          config::ThreadNumType threads_num);

    std::vector<MaxEpisodesCollection> Mine(std::vector<ParallelEpisode> const& seeds);

private:
    struct Context {
        boost::asio::thread_pool& pool;
        std::vector<ParallelEpisode> const& all_seeds;
        size_t min_support;
        size_t window_length;

        std::atomic<int> tasks_in_flight{0};
        int max_parallel_tasks;

        std::mutex results_mutex;
        std::vector<MaxEpisodesCollection> global_results;

        Context(boost::asio::thread_pool& thread_pool, std::vector<ParallelEpisode> const& seeds,
                size_t min, size_t win, config::ThreadNumType threads_num);

        void Commit(MaxEpisodesCollection&& local_buf);
    };

    void RunSearchTask(CompositeEpisode episode, BoundList bound_list, Context& ctx);

    size_t min_support_;
    size_t window_length_;
    config::ThreadNumType threads_num_;
};

}  // namespace algos::maxfem
