#pragma once

#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "algorithms/dc/model/tuple.h"
#include "model/table/column.h"
#include "model/table/relation_data.h"
#include "model/types/type.h"
#include "util/logger.h"

namespace algos::dc {

// @brief Represents a column operand for Predicate.
//
// A predicate (e.g., t.A == s.A) comprises three elements:
// the column operand from the first tuple ("t.A"), the comparison operator
// ("=="), and the column operand from the second tuple ("s.A"). The `ColumnOperand` class
// encapsulates the column operand part of a predicate, such as "t.A" or "s.A".
//
// If a ColumnOperand is a constant value, then column_
// is initialized with nullptr and tuple_ with std::nullopt,
// in this case accessing tuple_ and column_ from the outside of the class is not allowed.
// The type_ is set to the type of the other ColumnOperand in Predicate (should be variable).
//
// If a ColumnOperand involves any tuple (s or t) then it is a variable operand
// thus val_ is initialized with nullptr and is not accessible, type_ is set to
// the type of the column_, which is initialized corresponding to the given name or index.
//
// Example: s.Col0 > 1.5,
// In this case left operand is a variable operand thus val_ is nullptr, col_ is a pointer to Col0,
// type_ is set to the type of Col0, tuple_ is set to the given tuple (s -> Tuple::kS)
// Right operand is simply a constant value, thus it has no column and tuple,
// val_ is initialized with 1.5, type_ is initialized with the type of left ColumnOperand.

class ColumnOperand {
private:
    Column const* column_;
    std::optional<dc::Tuple> tuple_;
    model::Type const* type_;
    std::byte const* val_;

public:
    ColumnOperand() noexcept : column_(nullptr), val_(nullptr) {};

    ColumnOperand(Column const* column, dc::Tuple tuple, model::Type const* type)
        : column_(column), tuple_(tuple), type_(type), val_(nullptr) {}

    ColumnOperand(std::string const& str_val, model::Type const* type)
        : column_(nullptr), type_(type) {
        std::byte* val = type_->Allocate();
        type_->ValueFromStr(val, str_val);
        val_ = val;
    }

    ColumnOperand(ColumnOperand const& rhs) : type_(rhs.type_) {
        if (rhs.IsVariable()) {
            tuple_ = rhs.tuple_;
            column_ = rhs.column_;
            val_ = nullptr;
        } else {
            val_ = rhs.type_->Clone(rhs.val_);
            tuple_ = std::nullopt;
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
        std::swap(tuple_, rhs.tuple_);
    }

    bool operator==(ColumnOperand const& rhs) const {
        if (IsConstant() != rhs.IsConstant()) return false;

        if (IsConstant()) {
            assert(type_ == rhs.type_);
            return type_->Compare(GetVal(), rhs.GetVal()) == model::CompareResult::kEqual;
        }

        return column_ == rhs.column_ && tuple_ == rhs.tuple_;
    }

    bool operator!=(ColumnOperand const& rhs) const = default;

    Column const* GetColumn() const {
        assert(column_ != nullptr);
        return column_;
    }

    Tuple GetTuple() const {
        assert(tuple_.has_value());
        return tuple_.value();
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
            res = (tuple_.value() == Tuple::kT ? "t." : "s.") + column_->GetName();
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
