#pragma once

#include <mutex>

#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/vertical_map.h"

class PartitionStorage {
private:
    class PositionListIndexRank {
    public:
        Vertical const* vertical_;
        std::shared_ptr<model::PositionListIndex const> pli_;
        int added_arity_;

        PositionListIndexRank(Vertical const* vertical,
                              std::shared_ptr<model::PositionListIndex const> pli,
                              int initial_arity)
            : vertical_(vertical), pli_(pli), added_arity_(initial_arity) {}
    };

    ColumnLayoutRelationData* relation_data_;
    std::unique_ptr<model::VerticalMap<model::PositionListIndex const>> index_;

    mutable std::mutex getting_pli_mutex_;

    std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
    CachingProcess(Vertical const& vertical, std::unique_ptr<model::PositionListIndex const> pli);

public:
    PartitionStorage(ColumnLayoutRelationData* relation_data);

    model::PositionListIndex const* Get(Vertical const& vertical);
    std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
    GetOrCreateFor(Vertical const& vertical);

    virtual ~PartitionStorage();
};
