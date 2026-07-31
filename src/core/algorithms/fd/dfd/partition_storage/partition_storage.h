#pragma once

#include <mutex>

#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/vertical_map.h"

class PartitionStorage {
private:
    ColumnLayoutRelationData* relation_data_;
    std::unique_ptr<model::VerticalMap<model::PositionListIndex const>> index_;

    mutable std::mutex getting_pli_mutex_;

    std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
    CachingProcess(boost::dynamic_bitset<> const& vertical,
                   std::unique_ptr<model::PositionListIndex const> pli);

public:
    PartitionStorage(ColumnLayoutRelationData* relation_data);

    model::PositionListIndex const* Get(boost::dynamic_bitset<> const& vertical);
    std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
    GetOrCreateFor(boost::dynamic_bitset<> const& vertical);

    virtual ~PartitionStorage();
};
