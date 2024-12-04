#pragma once
#include <cstddef>
#include <utility>

#include "model/table/column_index.h"

class Highlight {
private:
    model::ColumnIndex attribute_index_;
    std::pair<std::size_t, std::size_t> pair_rows_;
    double distance_;

public:
    Highlight(model::ColumnIndex attribute_index, std::pair<std::size_t, std::size_t> rows_pair,
              double distance)
        : attribute_index_(attribute_index), pair_rows_(rows_pair), distance_(distance) {}

    [[nodiscard]] model::ColumnIndex GetAttributeIndex() const {
        return attribute_index_;
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> GetPairRows() const {
        return pair_rows_;
    }

    [[nodiscard]] double GetDistance() const {
        return distance_;
    }
};
