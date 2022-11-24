#pragma once

#include <memory>
#include <utility>

namespace model {

/* Хранит два значения из таблицы из разных колонок, их индексы в таблице
 * и результат проведения между ними арифметической операции */
struct AC {
public:
    struct ColumnValueIndex {
        size_t column;
        size_t value_index;
    };

private:
    /* Индекс левого операнда и его значение */
    ColumnValueIndex lhs;
    std::byte const* lhs_val;
    /* Индекс правого операнда и его значение */
    ColumnValueIndex rhs;
    std::byte const* rhs_val;
    /* Результат арифметической операции */
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
