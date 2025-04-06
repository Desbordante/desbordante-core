#pragma once
#include <cstddef>
#include <memory>
#include <utility>

#include "model/table/column_index.h"

class Highlight {
private:
    model::ColumnIndex attribute_index_;
    std::pair<std::size_t, std::size_t> pair_rows_;

public:
    Highlight(model::ColumnIndex attribute_index, std::pair<std::size_t, std::size_t> rows_pair)
        : attribute_index_(attribute_index), pair_rows_(std::move(rows_pair)) {}

    [[nodiscard]] model::ColumnIndex GetAttributeIndex() const {
        return attribute_index_;
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> GetPairRows() const {
        return pair_rows_;
    }
};