#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include "util/static_map.h"

namespace algos::dc {

enum class OperatorType { kEqual, kUnequal, kGreater, kLess, kGreaterEqual, kLessEqual };

class Operator {
private:
    OperatorType op_;

public:
    constexpr Operator(OperatorType type) noexcept : op_(type) {}

    Operator(std::string_view str_op) {
        OperatorType const* op_ptr = kStringToOperatorType.Find(str_op);
        if (op_ptr == nullptr) {
            throw std::invalid_argument("Unknown operator: " + std::string(str_op));
        }

        op_ = *op_ptr;
    }

    std::string ToString() const {
        std::string_view sv = kOperatorTypeToString.At(op_);
        return std::string(sv);
    }

    bool operator==(Operator const& rhs) const noexcept {
        return op_ == rhs.op_;
    }

    bool operator!=(Operator const& rhs) const noexcept {
        return op_ != rhs.op_;
    }

    OperatorType GetType() const noexcept {
        return op_;
    }

    static constexpr util::StaticMap<OperatorType, std::string_view, 6> kOperatorTypeToString{
            {{{OperatorType::kEqual, "=="},
              {OperatorType::kUnequal, "!="},
              {OperatorType::kGreater, ">"},
              {OperatorType::kLess, "<"},
              {OperatorType::kGreaterEqual, ">="},
              {OperatorType::kLessEqual, "<="}}}};

    static constexpr util::StaticMap<std::string_view, OperatorType, 6> kStringToOperatorType{
            {{{"==", OperatorType::kEqual},
              {"!=", OperatorType::kUnequal},
              {">", OperatorType::kGreater},
              {"<", OperatorType::kLess},
              {">=", OperatorType::kGreaterEqual},
              {"<=", OperatorType::kLessEqual}}}};
};

}  // namespace algos::dc
