#pragma once

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <easylogging++.h>

#include "model/table/column.h"
#include "model/table/relation_data.h"
#include "model/types/type.h"

namespace algos::dc {

enum class Tuple { kS, kT, kMixed };

//  @brief Represents a column operand for Predicate.
//
//  A predicate (e.g., t.A == s.A) comprises three elements:
//  the column operand from the first tuple ("t.A"), the comparison operator
//  ("=="), and the column operand from the second tuple ("s.A"). The `ColumnOperand` class
//  encapsulates the column operand part of a predicate, such as "t.A" or "s.A".
//
//  A constant value also can be a column operand thus boost::optional is utilized
//
//  The class distinguishes between operands derived from the first tuple (t) and those
//  from the second tuple (s) using a boolean flag `is_first_tuple_`, where `true` indicates an
//  operand from the first tuple (t), and `false` indicates an operand from the second
//  tuple (s).
class ColumnOperand {
private:
    boost::optional<Column const*> column_;
    boost::optional<dc::Tuple> tuple_;
    model::Type const* type_;
    std::byte const* val_;

public:
    ColumnOperand() noexcept : val_(nullptr) {};

    ColumnOperand(Column const* column, dc::Tuple tuple, model::Type const* type)
        : column_(column), tuple_(tuple), type_(type), val_(nullptr) {}

    ColumnOperand(std::string const& str_val, model::Type const* type) : type_(type) {
        std::byte* val = type->Allocate();
        type->ValueFromStr(val, str_val);
        val_ = val;
    }

    ColumnOperand(ColumnOperand const& rhs) : type_(rhs.type_) {
        if (rhs.IsVariable()) {
            tuple_ = rhs.tuple_;
            column_ = rhs.column_;
            val_ = nullptr;
        } else {
            val_ = rhs.type_->Clone(rhs.val_);
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

    bool operator!=(ColumnOperand const& rhs) const {
        return !(*this == rhs);
    }

    Column const* GetColumn() const {
        assert(column_.is_initialized());
        return column_.get();
    }

    Tuple GetTuple() const {
        assert(tuple_.is_initialized());
        return tuple_.get();
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
            res = (tuple_.get() == Tuple::kT ? "t." : "s.") + column_.get()->GetName();
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
