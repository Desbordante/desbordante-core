#pragma once

#include <list>
#include <mutex>

#include "algorithms/fd/pli_based_fd_algorithm.h"
#include "core/dependency_consumer.h"
#include "core/search_space.h"

namespace algos {

class Pyro : public DependencyConsumer, public PliBasedFDAlgorithm {
private:
    std::list<std::unique_ptr<SearchSpace>> search_spaces_;

    CachingMethod caching_method_ = CachingMethod::kCoin;
    CacheEvictionMethod eviction_method_ = CacheEvictionMethod::kDefault;
    double caching_method_value_;

    pyro::Parameters parameters_;

    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() final;

public:
    Pyro();
};

}  // namespace algos
