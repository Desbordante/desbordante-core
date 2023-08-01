#pragma once

#include <mutex>

#include "cache_eviction_method.h"
#include "caching_method.h"
#include "model/table/column_layout_relation_data.h"
#include "structures/vertical_map.h"

class PartitionStorage {
private:
    class PositionListIndexRank {
    public:
        Vertical const* vertical_;
        std::shared_ptr<structures::PositionListIndex> pli_;
        int added_arity_;

        PositionListIndexRank(Vertical const* vertical,
                              std::shared_ptr<structures::PositionListIndex> pli, int initial_arity)
            : vertical_(vertical), pli_(pli), added_arity_(initial_arity) {}
    };

    ColumnLayoutRelationData* relation_data_;
    std::unique_ptr<structures::VerticalMap<structures::PositionListIndex>> index_;

    int saved_intersections_ = 0;

    mutable std::mutex getting_pli_mutex_;

    CachingMethod caching_method_;
    CacheEvictionMethod eviction_method_;
    double caching_method_value_;

    double median_inverted_entropy_;

    std::variant<structures::PositionListIndex*, std::unique_ptr<structures::PositionListIndex>>
    CachingProcess(Vertical const& vertical, std::unique_ptr<structures::PositionListIndex> pli);

public:
    PartitionStorage(ColumnLayoutRelationData* relation_data, CachingMethod caching_method,
                     CacheEvictionMethod eviction_method);

    structures::PositionListIndex* Get(Vertical const& vertical);
    std::variant<structures::PositionListIndex*, std::unique_ptr<structures::PositionListIndex>>
    GetOrCreateFor(Vertical const& vertical);

    size_t Size() const;

    virtual ~PartitionStorage();
};
