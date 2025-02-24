#pragma once

#include <list>      // for list
#include <memory>    // for uniqu...
#include <optional>  // for optional

#include "algorithms/fd/pli_based_fd_algorithm.h"               // for PliBa...
#include "algorithms/fd/pyrocommon/core/dependency_consumer.h"  // for Depen...
#include "algorithms/fd/pyrocommon/core/search_space.h"         // for Searc...
#include "cache_eviction_method.h"                              // for Cache...
#include "caching_method.h"                                     // for Cachi...
#include "fd/pyrocommon/core/parameters.h"                      // for Param...

namespace algos {

/* Class for mining FD with pyro algorithm */
class Pyro : public DependencyConsumer, public PliBasedFDAlgorithm {
private:
    std::list<std::unique_ptr<SearchSpace>> search_spaces_;

    CachingMethod caching_method_ = CachingMethod::kCoin;
    CacheEvictionMethod eviction_method_ = CacheEvictionMethod::kDefault;
    double caching_method_value_;

    pyro::Parameters parameters_;

    void RegisterOptions();
    void MakeExecuteOptsAvailableFDInternal() final;

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() final;

public:
    Pyro(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
};

}  // namespace algos
