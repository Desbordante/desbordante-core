#pragma once

#include <memory>
#include <queue>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "all_column_combinations.h"
#include "efficiency_threshold.h"
#include "types.h"
#include "util/position_list_index.h"

namespace algos::hy {

class Sampler {
private:
    class Efficiency;
    double efficiency_threshold_ = kEfficiencyThreshold;

    PLIsPtr plis_;
    RowsPtr compressed_records_;
    std::priority_queue<Efficiency> efficiency_queue_;

    std::unique_ptr<AllColumnCombinations> agree_sets_;

    void ProcessComparisonSuggestions(IdPairs const& comparison_suggestions);
    void InitializeEfficiencyQueue();

    void Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
               size_t second_record_id);
    void RunWindow(Efficiency& efficiency, util::PositionListIndex const& pli);

public:
    Sampler(PLIsPtr plis, RowsPtr pli_records);

    Sampler(Sampler const& other) = delete;
    Sampler(Sampler&& other) = delete;
    Sampler& operator=(Sampler const& other) = delete;
    Sampler& operator=(Sampler&& other) = delete;
    ~Sampler();

    ColumnCombinationList GetAgreeSets(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hy
