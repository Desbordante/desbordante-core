#pragma once

#include <cassert>
#include <set>
#include <vector>

#include "abstract_column_data.h"
#include "column.h"
#include "table_index.h"
#include "value_dictionary.h"

namespace model {
class EncodedColumnData final : public model::AbstractColumnData {
public:
    EncodedColumnData(TableIndex table_id, Column const* column, std::vector<int> column_data,
                      std::set<std::string> unique_values,
                      std::shared_ptr<ValueDictionary> value_dictionary)
        : AbstractColumnData(column),
          table_id_(table_id),
          column_data_(std::move(column_data)),
          unique_values_(std::move(unique_values)),
          value_dictionary_(std::move(value_dictionary)) {}

    std::string ToString() const final {
        return "Data for " + column_->ToString();
    }

    std::string const& GetStringValue(size_t index) const {
        assert(index < column_data_.size());
        return value_dictionary_->ToString(column_data_.at(index));
    }

    int GetValue(size_t index) const {
        assert(index < column_data_.size());
        return column_data_.at(index);
    }

    std::vector<int> const& GetValues() const noexcept {
        return column_data_;
    }

    [[nodiscard]] size_t GetNumRows() const noexcept {
        return column_data_.size();
    }

    TableIndex GetTableId() const noexcept {
        return table_id_;
    }

    model::ColumnIndex GetColumnId() const noexcept {
        return column_->GetIndex();
    }

    std::set<std::string> const& GetUniqueValues() const noexcept {
        return unique_values_;
    }

    std::string DecodeValue(int value) const {
        return value_dictionary_->ToString(value);
    }

private:
    TableIndex table_id_;
    std::vector<int> column_data_;
    std::set<std::string> unique_values_;
    std::shared_ptr<ValueDictionary> value_dictionary_;
};
}  // namespace model
