#pragma once
#include <cstddef>

namespace algos::cdd {

class CDDHighlight {
private:
    std::size_t row_index_;
    std::size_t condition_index_;

public:
    CDDHighlight(std::size_t row_index, std::size_t condition_index)
        : row_index_(row_index), condition_index_(condition_index) {}

    [[nodiscard]] std::size_t GetRowIndex() const {
        return row_index_;
    }

    [[nodiscard]] std::size_t GetConditionIndex() const {
        return condition_index_;
    }
};

}  // namespace algos::cdd
