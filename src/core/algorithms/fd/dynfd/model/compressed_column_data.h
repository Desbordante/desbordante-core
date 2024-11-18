#pragma once

#include <memory>

#include "dynamic_position_list_index.h"
#include "model/table/abstract_column_data.h"

namespace algos::dynfd {

class CompressedColumnData : model::AbstractColumnData {
    std::shared_ptr<DynamicPositionListIndex> position_list_index_;

public:
    CompressedColumnData(Column const* column,
                         std::unique_ptr<DynamicPositionListIndex> position_list_index)
        : AbstractColumnData(column), position_list_index_(std::move(position_list_index)) {}

    [[nodiscard]] size_t GetNumRows() const {
        return position_list_index_->GetSize();
    }

    [[nodiscard]] std::string ToString() const final {
        return "Data for " + column_->ToString();
    }

    [[nodiscard]] std::shared_ptr<DynamicPositionListIndex> GetPositionListIndex() const {
        return position_list_index_;
    }
};

}  // namespace algos::dynfd
