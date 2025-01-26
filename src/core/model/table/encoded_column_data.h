#pragma once

#include <cassert>
#include <unordered_map>
#include <vector>

#include "abstract_column_data.h"
#include "column.h"
#include "table/table_index.h"

namespace model {
class EncodedColumnData final : public model::AbstractColumnData {
public:
    EncodedColumnData(TableIndex table_id, Column const* column, std::vector<int> column_data,
                      std::shared_ptr<std::unordered_map<int, std::string>> value_dictionary)
        : AbstractColumnData(column),
          table_id_(table_id),
          column_data_(std::move(column_data)),
          value_dictionary_(std::move(value_dictionary)) {}

    std::string ToString() const final {
        return "Data for " + column_->ToString();
    }

    std::string const& GetStringValue(size_t index) const {
        assert(index < column_data_.size());
        return value_dictionary_->at(column_data_.at(index));
    }

    int GetValue(size_t index) const {
        assert(index < column_data_.size());
        return column_data_.at(index);
    }

    [[nodiscard]] size_t GetNumRows() const noexcept {
        return column_data_.size();
    }

    TableIndex GetTableId() const noexcept {
        return table_id_;
    }

    TableIndex GetColumnId() const noexcept {
        return column_->GetIndex();
    }

private:
    TableIndex table_id_;
    std::vector<int> column_data_;
    std::shared_ptr<std::unordered_map<int, std::string>> value_dictionary_;
};
}  // namespace model
