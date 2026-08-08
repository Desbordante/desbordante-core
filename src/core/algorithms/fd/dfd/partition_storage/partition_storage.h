#pragma once

#include <cstddef>
#include <mutex>

#include "core/model/table/position_list_index.h"
#include "core/model/table/vertical_map.h"

class PartitionStorage {
private:
    std::vector<model::PositionListIndex> const* input_table_column_plis_;
    std::unique_ptr<model::VerticalMap<model::PositionListIndex const>> index_;

    mutable std::mutex getting_pli_mutex_;

    model::PositionListIndex const* CachingProcess(
            boost::dynamic_bitset<> const& vertical,
            std::unique_ptr<model::PositionListIndex const> pli);

public:
    PartitionStorage(std::vector<model::PositionListIndex> const& input_table_column_plis);

    model::PositionListIndex const* Get(boost::dynamic_bitset<> const& vertical);
    std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
    GetOrCreateFor(boost::dynamic_bitset<> const& vertical);

    virtual ~PartitionStorage();
};
