#pragma once

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <boost/functional/hash.hpp>
#include <easylogging++.h>

#include "model/table/column.h"
#include "model/table/relation_data.h"

namespace algos::dc {

// @brief Represents a column operand for Predicate.
//
// A predicate (e.g., t.A == s.A) comprises three elements:
// the column operand from the first tuple ("t.A"), the comparison operator
// ("=="), and the column operand from the second tuple ("s.A"). The `ColumnOperand` class
// encapsulates the column operand part of a predicate, such as "t.A" or "s.A".
//
// The class distinguishes between operands derived from the first tuple (t) and those
// from the second tuple (s) using a boolean flag `is_first_tuple_`, where `true` indicates an
// operand from the first tuple (t), and `false` indicates an operand from the second tuple (s).
//
class ColumnOperand {
private:
    Column const* column_;
    bool is_first_tuple_;

public:
    ColumnOperand(Column const* column, bool is_first_tuple) noexcept
        : column_(column), is_first_tuple_(is_first_tuple) {}

    ColumnOperand() noexcept = default;

    // For conversion from "t.ColumnPosition" or "t.ColumnName"
    ColumnOperand(std::string operand, RelationalSchema const& schema) {
        if (operand.front() != 't' and operand.front() != 's')
            throw std::logic_error("Unknown tuple name");

        is_first_tuple_ = operand.front() == 't';
        std::string name(operand.begin() + 2, operand.end());
        std::vector<std::unique_ptr<Column>> const& cols = schema.GetColumns();
        if (!cols.front()->GetName().empty()) {  // Has header
            for (std::unique_ptr<Column> const& col : cols) {
                if (name == col->GetName()) {
                    column_ = col.get();
                    return;
                }
            }
        }

        std::string str_ind(operand.begin() + 2, operand.end());
        model::ColumnIndex ind = std::stoi(str_ind);
        column_ = cols[ind].get();
    }

    bool operator==(ColumnOperand const& rhs) const noexcept {
        return column_ == rhs.column_ && is_first_tuple_ == rhs.is_first_tuple_;
    }

    bool operator!=(ColumnOperand const& rhs) const noexcept {
        return !(*this == rhs);
    }

    Column const* GetColumn() const noexcept {
        return column_;
    }

    bool IsFirstTuple() const noexcept {
        return is_first_tuple_;
    }

    std::string ToString() const noexcept {
        return (is_first_tuple_ ? "t." : "s.") + column_->GetName();
    }
};

}  // namespace algos::dc
