#pragma once

#include <stdexcept>
#include <string>

#include <frozen/string.h>
#include <frozen/unordered_map.h>

namespace algos::dc {

enum class OperatorType { kEqual, kUnequal, kGreater, kLess, kGreaterEqual, kLessEqual };

class Operator {
private:
    OperatorType op_;

public:
    constexpr Operator(OperatorType type) noexcept : op_(type) {}

    Operator(std::string str_op) {
        frozen::string s = {str_op.data(), str_op.size()};
        auto it = kStringToOperatorType.find(s);
        if (it == kStringToOperatorType.end()) throw std::invalid_argument("Unknown operator");
        op_ = it->second;
    }

    std::string ToString() const {
        frozen::string str = kOperatorTypeToString.at(op_);
        return str.data();
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

    static constexpr frozen::unordered_map<OperatorType, frozen::string, 6> kOperatorTypeToString{
            {OperatorType::kEqual, "=="},        {OperatorType::kUnequal, "!="},
            {OperatorType::kGreater, ">"},       {OperatorType::kLess, "<"},
            {OperatorType::kGreaterEqual, ">="}, {OperatorType::kLessEqual, "<="}};

    static constexpr frozen::unordered_map<frozen::string, OperatorType, 6> kStringToOperatorType{
            {"==", OperatorType::kEqual},        {"!=", OperatorType::kUnequal},
            {">", OperatorType::kGreater},       {"<", OperatorType::kLess},
            {">=", OperatorType::kGreaterEqual}, {"<=", OperatorType::kLessEqual}};
};

}  // namespace algos::dc
