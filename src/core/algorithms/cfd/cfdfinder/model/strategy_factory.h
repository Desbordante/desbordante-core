#pragma once

#include <cstddef>
#include <memory>

#include "core/algorithms/cfd/cfdfinder/enums.h"
#include "core/algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/result/result_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"
#include "core/algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"
#include "core/config/thread_number/type.h"

namespace algos::cfdfinder {

struct PruningStrategyConfig {
    size_t num_rows;
    double min_support;
    double min_confidence;
    double max_g1;
    size_t max_patterns;
    double min_support_gain;
    double max_level_support_drop;
    config::ThreadNumType thread_num;
};

std::shared_ptr<ExpansionStrategy> CreateExpansionStrategy(
        Expansion strategy, RowsPtr pli_records, InvertedClusterMaps const& inverted_cluster_maps);

std::shared_ptr<PruningStrategy> CreatePruningStrategy(Pruning strategy,
                                                       PruningStrategyConfig&& config,
                                                       ColumnsPtr inverted_plis);

std::shared_ptr<ResultStrategy> CreateResultStrategy(Result strategy);

}  // namespace algos::cfdfinder
