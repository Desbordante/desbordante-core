#pragma once

#include <memory>
#include <queue>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "all_column_combinations.h"
#include "config/thread_number/type.h"
#include "efficiency_threshold.h"
#include "structures/position_list_index.h"
#include "types.h"

namespace boost::asio {
// Forward declare thread_pool to avoid including boost::asio::thread_pool implementation since
// it's not needed here and to avoid transitevly pollute all other files with it
class thread_pool;
}  // namespace boost::asio

namespace algos::hy {

class Sampler {
private:
    class Efficiency;
    double efficiency_threshold_ = kEfficiencyThreshold;

    PLIsPtr plis_;
    RowsPtr compressed_records_;
    std::priority_queue<Efficiency> efficiency_queue_;
    std::unique_ptr<AllColumnCombinations> agree_sets_;
    util::config::ThreadNumType threads_num_;
    std::unique_ptr<boost::asio::thread_pool> pool_;

    void ProcessComparisonSuggestions(IdPairs const& comparison_suggestions);
    void SortClustersSeq();
    void SortClustersParallel();
    void SortClusters();
    void InitializeEfficiencyQueueSeq();
    void InitializeEfficiencyQueueParallel();
    void InitializeEfficiencyQueueImpl();
    void InitializeEfficiencyQueue();

    void Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
               size_t second_record_id);
    template <typename F>
    void RunWindowImpl(Efficiency& efficiency, util::PositionListIndex const& pli, F store_match);
    std::vector<boost::dynamic_bitset<>> RunWindowRet(Efficiency& efficiency,
                                                      util::PositionListIndex const& pli);
    void RunWindow(Efficiency& efficiency, util::PositionListIndex const& pli);

public:
    Sampler(PLIsPtr plis, RowsPtr pli_records, util::config::ThreadNumType threads = 1);

    Sampler(Sampler const& other) = delete;
    Sampler(Sampler&& other) = delete;
    Sampler& operator=(Sampler const& other) = delete;
    Sampler& operator=(Sampler&& other) = delete;
    ~Sampler();

    ColumnCombinationList GetAgreeSets(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hy
