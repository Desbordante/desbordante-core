#pragma once

#include <memory>  // for uniqu...

#include "algorithms/fd/pyrocommon/core/dependency_consumer.h"  // for Depen...
#include "algorithms/fd/pyrocommon/core/search_space.h"         // for Searc...
#include "cache_eviction_method.h"                              // for Cache...
#include "caching_method.h"                                     // for Cachi...
#include "fd/pyrocommon/core/parameters.h"                      // for Param...
#include "table/column_layout_relation_data.h"                  // for Colum...
#include "ucc/ucc_algorithm.h"                                  // for UCCAl...

namespace algos {

class PyroUCC : public DependencyConsumer, public UCCAlgorithm {
private:
    std::unique_ptr<ColumnLayoutRelationData> relation_;

    std::unique_ptr<SearchSpace> search_space_;

    CachingMethod caching_method_ = CachingMethod::kCoin;
    CacheEvictionMethod eviction_method_ = CacheEvictionMethod::kDefault;
    double caching_method_value_;

    pyro::Parameters parameters_;

    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;

    void ResetUCCAlgorithmState() final;
    unsigned long long ExecuteInternal() final;
    void LoadDataInternal() final;

public:
    PyroUCC();
};

}  // namespace algos
