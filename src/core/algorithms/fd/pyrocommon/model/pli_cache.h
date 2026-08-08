#pragma once

class ProfilingContext;

#include <mutex>

#include "core/algorithms/fd/pyrocommon/core/profiling_context.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/util/cache_eviction_method.h"
#include "core/util/caching_method.h"
#include "core/util/maybe_unused_private_field.h"

namespace model {

class PLICache {
private:
    ColumnLayoutRelationData* relation_data_;
    std::unique_ptr<VerticalMap<PositionListIndex const>> index_;
    // usageCounter - for parallelism

    // All these MAYBE_UNUSED_PRIVATE_FIELD variables are required to support Pyro's caching
    // strategies from our ADBIS paper:
    // https://link.springer.com/chapter/10.1007/978-3-030-30278-8_7

    MAYBE_UNUSED_PRIVATE_FIELD int saved_intersections_ = 0;

    mutable std::mutex getting_pli_mutex_;

    CachingMethod caching_method_;
    MAYBE_UNUSED_PRIVATE_FIELD CacheEvictionMethod eviction_method_;
    MAYBE_UNUSED_PRIVATE_FIELD double caching_method_value_;
    // long long maximumAvailableMemory_ = 0;
    double maximum_entropy_;
    MAYBE_UNUSED_PRIVATE_FIELD double mean_entropy_;
    MAYBE_UNUSED_PRIVATE_FIELD double min_entropy_;
    MAYBE_UNUSED_PRIVATE_FIELD double median_entropy_;
    MAYBE_UNUSED_PRIVATE_FIELD double median_gini_;
    MAYBE_UNUSED_PRIVATE_FIELD double median_inverted_entropy_;

    std::variant<PositionListIndex const*, std::unique_ptr<PositionListIndex const>> CachingProcess(
            boost::dynamic_bitset<> const& vertical, std::unique_ptr<PositionListIndex const> pli,
            ProfilingContext* profiling_context);

public:
    PLICache(ColumnLayoutRelationData* relation_data, CachingMethod caching_method,
             CacheEvictionMethod eviction_method, double caching_method_value, double min_entropy,
             double mean_entropy, double median_entropy, double maximum_entropy, double median_gini,
             double median_inverted_entropy);

    PositionListIndex const* Get(boost::dynamic_bitset<> const& vertical);
    std::variant<PositionListIndex const*, std::unique_ptr<PositionListIndex const>> GetOrCreateFor(
            boost::dynamic_bitset<> const& vertical, ProfilingContext* profiling_context);

    void SetMaximumEntropy(double e) {
        maximum_entropy_ = e;
    }

    // returns ownership of single column PLIs back to ColumnLayoutRelationData
    virtual ~PLICache();
};

}  // namespace model
