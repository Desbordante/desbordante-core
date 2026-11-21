#include "core/algorithms/cfd/cfdfinder/model/strategy_factory.h"

#include <utility>

#include "core/algorithms/cfd/cfdfinder/model/expansion/constant_expansion_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/expansion/positive_negative_constant_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/expansion/range_pattern_expansion.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/legacy_pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/partial_fd_pruning.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/support_independent_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/result/direct_output_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/result/result_lattice_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/result/result_tree_strategy.h"

namespace algos::cfdfinder {

std::shared_ptr<ExpansionStrategy> CreateExpansionStrategy(
        Expansion strategy, RowsPtr pli_records, InvertedClusterMaps const& inverted_cluster_maps) {
    switch (strategy) {
        case Expansion::kConstant:
            return std::make_shared<ConstantExpansion>(std::move(pli_records));
        case Expansion::kRange:
            return std::make_shared<RangePatternExpansion>(inverted_cluster_maps,
                                                           std::move(pli_records));
        case Expansion::kNegativeConstant:
            return std::make_shared<PositiveNegativeConstantExpansion>(std::move(pli_records));
        default:
            return std::make_shared<ConstantExpansion>(std::move(pli_records));
    }
}

std::shared_ptr<PruningStrategy> CreatePruningStrategy(Pruning strategy,
                                                       PruningStrategyConfig&& config,
                                                       ColumnsPtr inverted_plis) {
    switch (strategy) {
        case Pruning::kLegacy:
            return std::make_shared<LegacyPruning>(config.min_support, config.min_confidence,
                                                   config.num_rows);
        case Pruning::kSupportIndependent:
            return std::make_shared<SupportIndependentPruning>(
                    config.max_patterns, config.min_support_gain, config.max_level_support_drop,
                    config.min_confidence, config.thread_num);
        case Pruning::kPartialFd:
            return std::make_shared<PartialFdPruning>(config.num_rows, config.max_g1,
                                                      std::move(inverted_plis));
        default:
            return std::make_shared<LegacyPruning>(config.min_support, config.min_confidence,
                                                   config.num_rows);
    }
}

std::shared_ptr<ResultStrategy> CreateResultStrategy(Result strategy) {
    switch (strategy) {
        case Result::kDirect:
            return std::make_shared<DirectOutputStrategy>();
        case Result::kLattice:
            return std::make_shared<ResultLatticeStrategy>();
        case Result::kTree:
            return std::make_shared<ResultTreeStrategy>();
        default:
            return std::make_shared<DirectOutputStrategy>();
    }
}

}  // namespace algos::cfdfinder
