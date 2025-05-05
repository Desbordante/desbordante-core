#pragma once

#include <memory>    // for unique_ptr
#include <queue>     // for priority_queue
#include <stddef.h>  // for size_t
#include <vector>    // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset

#include "config/thread_number/type.h"            // for ThreadNumType
#include "efficiency_threshold.h"                 // for kEfficiencyThreshold
#include "fd/hycommon/column_combination_list.h"  // for ColumnCombination...
#include "types.h"                                // for IdPairs, PLIsPtr

namespace algos {
namespace hy {
class AllColumnCombinations;
}
}  // namespace algos

namespace model {
class PositionListIndex;
}

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
    config::ThreadNumType threads_num_;
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
    void RunWindowImpl(Efficiency& efficiency, model::PositionListIndex const& pli, F store_match);
    std::vector<boost::dynamic_bitset<>> RunWindowRet(Efficiency& efficiency,
                                                      model::PositionListIndex const& pli);
    void RunWindow(Efficiency& efficiency, model::PositionListIndex const& pli);

public:
    Sampler(PLIsPtr plis, RowsPtr pli_records, config::ThreadNumType threads = 1);

    Sampler(Sampler const& other) = delete;
    Sampler(Sampler&& other) = delete;
    Sampler& operator=(Sampler const& other) = delete;
    Sampler& operator=(Sampler&& other) = delete;
    ~Sampler();

    ColumnCombinationList GetAgreeSets(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hy
