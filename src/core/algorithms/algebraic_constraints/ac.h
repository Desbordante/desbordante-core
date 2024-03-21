#pragma once

#include <memory>

namespace algos {

/* Stores pair of values, indexes of values in the table
 * and the result of an arithmetic operation between them */
class ACPair {
public:
    struct ColumnValueIndex {
        size_t column;
        size_t value_index;

        bool operator==(ColumnValueIndex const& o) const {
            return column == o.column && value_index == o.value_index;
        }

        bool operator!=(ColumnValueIndex const& o) const {
            return !(o == *this);
        }
    };

private:
    /* Index of the left operand and its value */
    ColumnValueIndex lhs_;
    std::byte const* lhs_val_;
    /* Index of the right operand and its value */
    ColumnValueIndex rhs_;
    std::byte const* rhs_val_;
    /* The result of a binary arithmetic operation */
    std::unique_ptr<std::byte[]> res_;

public:
    ACPair(ColumnValueIndex l, ColumnValueIndex r, std::byte const* la, std::byte const* ra,
           std::unique_ptr<std::byte[]> res)
        : lhs_(l), lhs_val_(la), rhs_(r), rhs_val_(ra), res_(std::move(res)) {}

    ColumnValueIndex GetLhsColumnValueIndex() const {
        return lhs_;
    }

    ColumnValueIndex GetRhsColumnValueIndex() const {
        return rhs_;
    }

    std::byte const* GetLhsValue() const {
        return lhs_val_;
    }

    std::byte const* GetRhsValue() const {
        return rhs_val_;
    }

    std::byte const* GetRes() const {
        return res_.get();
    }
};

}  // namespace algos
