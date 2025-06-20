#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <thread>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/fd/hycommon/all_column_combinations.h"
#include "config/thread_number/type.h"
#include "algorithms/fd/hycommon/efficiency.h"
#include "model/dynamic_position_list_index.h"
#include "model/dynamic_relation_data.h"

namespace boost::asio {
// Forward declare thread_pool to avoid including boost::asio::thread_pool implementation since
// it's not needed here and to avoid transitevly pollute all other files with it
class thread_pool;
}  // namespace boost::asio

namespace algos::dynfd {

class Sampler {
public:
    using IdPairs = std::vector<std::pair<int, int>>;

private:
    using PLI = std::vector<DPLI::Cluster>;

    static inline constexpr double kEfficiencyThreshold = 0.1;

    double efficiency_threshold_ = kEfficiencyThreshold;
    std::vector<PLI> plis_;
    CompressedRecordsPtr compressed_records_;
    std::shared_ptr<DynamicRelationData> relation_;
    std::priority_queue<algos::hy::Efficiency> efficiency_queue_;
    std::unique_ptr<algos::hy::AllColumnCombinations> agree_sets_;
    boost::asio::thread_pool *pool_;
    size_t first_insert_batch_id;

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
    void RunWindowImpl(algos::hy::Efficiency& efficiency, PLI const& pli, F store_match);
    std::vector<boost::dynamic_bitset<>> RunWindowRet(algos::hy::Efficiency& efficiency,
                                                      PLI const& pli);
    void RunWindow(algos::hy::Efficiency& efficiency, PLI const& pli);

public:
    Sampler(std::shared_ptr<DynamicRelationData> relation,
            size_t first_insert_batch_id,
            boost::asio::thread_pool *pool);

    algos::hy::ColumnCombinationList GetAgreeSets(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hy
