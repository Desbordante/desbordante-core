#pragma once

#include <memory>

namespace model {

/* Stores two values from a table from two different columns,
 * indexes of those values in the table and the result
 * of an arithmetic operation between them */
struct AC {
public:
    struct ColumnValueIndex {
        size_t column;
        size_t value_index;
    };

private:
    /* Index of the left operand and its value */
    ColumnValueIndex lhs;
    std::byte const* lhs_val;
    /* Index of the right operand and its value */
    ColumnValueIndex rhs;
    std::byte const* rhs_val;
    /* The result of a binary arithmetic operation */
    std::unique_ptr<std::byte[]> res;

public:
    AC(ColumnValueIndex l, ColumnValueIndex r, std::byte const* la, std::byte const* ra,
       std::unique_ptr<std::byte[]> res)
        : lhs(l), lhs_val(la), rhs(r), rhs_val(ra), res(std::move(res)) {}

    ColumnValueIndex GetLhsColumnValueIndex() const {
        return lhs;
    }
    ColumnValueIndex GetRhsColumnValueIndex() const {
        return rhs;
    }
    std::byte const* GetLhsValue() const {
        return lhs_val;
    }
    std::byte const* GetRhsValue() const {
        return rhs_val;
    }
    std::byte const* GetRes() const {
        return res.get();
    }
};

}  // namespace model
