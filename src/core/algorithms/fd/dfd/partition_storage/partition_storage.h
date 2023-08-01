#pragma once

#include <mutex>

#include "cache_eviction_method.h"
#include "caching_method.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/vertical_map.h"

class PartitionStorage {
private:
    class PositionListIndexRank {
    public:
        Vertical const* vertical_;
        std::shared_ptr<model::PositionListIndex> pli_;
        int added_arity_;

        PositionListIndexRank(Vertical const* vertical,
                              std::shared_ptr<model::PositionListIndex> pli, int initial_arity)
            : vertical_(vertical), pli_(pli), added_arity_(initial_arity) {}
    };

    ColumnLayoutRelationData* relation_data_;
    std::unique_ptr<model::VerticalMap<model::PositionListIndex>> index_;

    int saved_intersections_ = 0;

    mutable std::mutex getting_pli_mutex_;

    CachingMethod caching_method_;
    CacheEvictionMethod eviction_method_;
    double caching_method_value_;

    double median_inverted_entropy_;

    std::variant<model::PositionListIndex*, std::unique_ptr<model::PositionListIndex>>
    CachingProcess(Vertical const& vertical, std::unique_ptr<model::PositionListIndex> pli);

public:
    PartitionStorage(ColumnLayoutRelationData* relation_data, CachingMethod caching_method,
                     CacheEvictionMethod eviction_method);

    model::PositionListIndex* Get(Vertical const& vertical);
    std::variant<model::PositionListIndex*, std::unique_ptr<model::PositionListIndex>>
    GetOrCreateFor(Vertical const& vertical);

    size_t Size() const;

    virtual ~PartitionStorage();
};
