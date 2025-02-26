#pragma once

#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <easylogging++.h>

#include "model/table/column.h"
#include "model/table/relation_data.h"
#include "model/types/type.h"

namespace algos::dc {

// @brief Represents a column operand for Predicate.
//
// A predicate (e.g., t.A == s.A) comprises three elements:
// the column operand from the first tuple ("t.A"), the comparison operator
// ("=="), and the column operand from the second tuple ("s.A"). The `ColumnOperand` class
// encapsulates the column operand part of a predicate, such as "t.A" or "s.A".
// 
// A constant value also can be a column operand thus std::optional is utilized
//
// The class distinguishes between operands derived from the first tuple (t) and those
// from the second tuple (s) using a boolean flag `is_first_tuple_`, where `true` indicates an
// operand from the first tuple (t), and `false` indicates an operand from the second tuple (s).
// 
class ColumnOperand {
private:
    std::optional<Column const*> column_;
    std::optional<bool> is_first_tuple_;
    model::Type const* type_;
    std::byte const* val_;

public:
    ColumnOperand() noexcept : val_(nullptr){};

    ColumnOperand(Column const* column, bool is_first_tuple, model::Type const* type)
        : column_(column), is_first_tuple_(is_first_tuple), type_(type), val_(nullptr) {}

    ColumnOperand(std::string const& str_val, model::Type const* type) : type_(type) {
        std::byte* val = type_->Allocate();
        type_->ValueFromStr(val, str_val);
        val_ = val;
    }

    ColumnOperand(ColumnOperand const& rhs) : type_(rhs.type_) {
        if (rhs.IsVariable()) {
            is_first_tuple_ = rhs.is_first_tuple_;
            column_ = rhs.column_;
            val_ = nullptr;
        } else {
            val_ = rhs.type_->Clone(rhs.val_);
            column_ = nullptr;
        }
    }

    ColumnOperand(ColumnOperand&& rhs) : ColumnOperand() {
        Swap(rhs);
    }

    ColumnOperand& operator=(ColumnOperand rhs) {
        Swap(rhs);
        return *this;
    }

    void Swap(ColumnOperand& rhs) {
        std::swap(type_, rhs.type_);
        std::swap(val_, rhs.val_);
        std::swap(column_, rhs.column_);
        std::swap(is_first_tuple_, rhs.is_first_tuple_);
    }

    bool operator==(ColumnOperand const& rhs) const {
        if (IsConstant() != rhs.IsConstant()) return false;

        if (IsConstant()) {
            assert(type_ == rhs.type_);
            return type_->Compare(GetVal(), rhs.GetVal()) == model::CompareResult::kEqual;
        }

        return GetColumn() == rhs.GetColumn() && IsFirstTuple() == rhs.IsFirstTuple();
    }

    bool operator!=(ColumnOperand const& rhs) const = default;

    Column const* GetColumn() const {
        assert(column_.has_value());
        return column_.value();
    }

    bool IsFirstTuple() const {
        assert(is_first_tuple_.has_value());
        return is_first_tuple_.value();
    }

    model::Type const* GetType() const noexcept {
        return type_;
    }

    std::byte const* GetVal() const {
        assert(val_ != nullptr);
        return val_;
    }

    bool IsConstant() const {
        return val_ != nullptr;
    }

    bool IsVariable() const {
        return val_ == nullptr;
    }

    std::string ToString() const {
        std::string res;
        if (IsVariable()) {
            res = (is_first_tuple_.value() ? "t." : "s.") + column_.value()->GetName();
        } else {
            res = type_->ValueToString(val_);
        }

        return res;
    }

    ~ColumnOperand() {
        if (val_ != nullptr) {
            type_->Free(val_);
        }
    }
};

}  // namespace algos::dc
