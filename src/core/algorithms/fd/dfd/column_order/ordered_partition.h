#pragma once

namespace model {
class PositionListIndex;
}

class OrderedPartition {
private:
    double distinctiveness_;
    unsigned int column_index_;

public:
    OrderedPartition() = delete;
    OrderedPartition(OrderedPartition const& other) = delete;
    OrderedPartition(OrderedPartition&& other) = delete;
    OrderedPartition& operator=(OrderedPartition const& other) = delete;
    OrderedPartition& operator=(OrderedPartition&& other) = delete;

    OrderedPartition(model::PositionListIndex const* const, unsigned int number_of_rows,
                     unsigned int column_index);

    bool operator<(OrderedPartition const& other) const;

    unsigned int GetColumnIndex() const {
        return this->column_index_;
    }
};
