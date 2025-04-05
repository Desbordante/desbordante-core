#pragma once
#include "model/table/column_index.h"
#include <cstddef>
#include <memory>
#include <utility>

class  Highlight {
    private:
        model::ColumnIndex attribute_index;
        std::pair<std::size_t, std::size_t> pair_rows;
    public:
        Highlight(model::ColumnIndex attribute_index, std::pair<std::size_t, std::size_t> rows_pair): attribute_index(attribute_index), pair_rows(std::move(rows_pair)) {}
        [[nodiscard]] model::ColumnIndex GetAttributeIndex() const { return attribute_index; }
        [[nodiscard]] std::pair<std::size_t, std::size_t> GetPairRows() const { return pair_rows; }
};