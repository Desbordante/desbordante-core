#pragma once

class ProfilingContext;

#include <mutex>

#include "../core/profiling_context.h"
#include "cache_eviction_method.h"
#include "caching_method.h"
#include "model/table/column_layout_relation_data.h"
#include "util/maybe_unused_private_field.h"

namespace model {

class PLICache {
private:
    class PositionListIndexRank {
    public:
        Vertical const* vertical_;
        std::shared_ptr<PositionListIndex> pli_;
        int added_arity_;

        PositionListIndexRank(Vertical const* vertical, std::shared_ptr<PositionListIndex> pli,
                              int initial_arity)
            : vertical_(vertical), pli_(pli), added_arity_(initial_arity) {}
    };

    // using CacheMap = VerticalMap<PositionListIndex>;
    ColumnLayoutRelationData* relation_data_;
    std::unique_ptr<VerticalMap<PositionListIndex>> index_;
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

    std::variant<PositionListIndex*, std::unique_ptr<PositionListIndex>> CachingProcess(
            Vertical const& vertical, std::unique_ptr<PositionListIndex> pli,
            ProfilingContext* profiling_context);

public:
    PLICache(ColumnLayoutRelationData* relation_data, CachingMethod caching_method,
             CacheEvictionMethod eviction_method, double caching_method_value, double min_entropy,
             double mean_entropy, double median_entropy, double maximum_entropy, double median_gini,
             double median_inverted_entropy);

    PositionListIndex* Get(Vertical const& vertical);
    std::variant<PositionListIndex*, std::unique_ptr<PositionListIndex>> GetOrCreateFor(
            Vertical const& vertical, ProfilingContext* profiling_context);

    void SetMaximumEntropy(double e) {
        maximum_entropy_ = e;
    }

    size_t Size() const;

    // returns ownership of single column PLIs back to ColumnLayoutRelationData
    virtual ~PLICache();
};

}  // namespace model
