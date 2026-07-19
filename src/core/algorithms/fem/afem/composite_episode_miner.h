#pragma once

#include <mutex>
#include <vector>

#include <boost/asio/thread_pool.hpp>

#include "core/algorithms/fem/maxfem/model/bound_list.h"
#include "core/algorithms/fem/maxfem/model/composite_episode.h"
#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/config/thread_number/type.h"

namespace algos::afem {

class CompositeEpisodeMiner {
public:
    CompositeEpisodeMiner(size_t min_support, size_t window_length,
                          config::ThreadNumType threads_num, double tasks_num_multiplier);

    std::vector<std::vector<maxfem::CompositeEpisode>> Mine(
            std::vector<maxfem::ParallelEpisode> const& seeds);

private:
    struct Context {
        boost::asio::thread_pool& pool;
        std::vector<maxfem::ParallelEpisode> const& all_seeds;
        size_t min_support;
        size_t window_length;

        std::atomic<int> tasks_in_flight{0};
        int max_parallel_tasks;

        std::mutex results_mutex;
        std::vector<std::vector<maxfem::CompositeEpisode>> global_results;

        Context(boost::asio::thread_pool& thread_pool,
                std::vector<maxfem::ParallelEpisode> const& seeds, size_t min, size_t win,
                int tasks_num);

        void Commit(std::vector<maxfem::CompositeEpisode>&& local_buf);
    };

    void RunSearchTask(maxfem::CompositeEpisode episode, maxfem::BoundList bound_list,
                       Context& ctx);

    size_t min_support_;
    size_t window_length_;
    config::ThreadNumType threads_num_;
    double tasks_num_multiplier_;
};

}  // namespace algos::afem
