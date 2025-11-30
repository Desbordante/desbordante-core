#pragma once

#include <list>
#include <mutex>

#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/algorithms/fd/pyrocommon/core/dependency_consumer.h"
#include "core/algorithms/fd/pyrocommon/core/search_space.h"
#include "core/algorithms/ucc/ucc_algorithm.h"

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
