#pragma once

#include "frozen/string.h"
#include "frozen/unordered_map.h"
#include "model/types/type.h"

namespace model {

enum class OperatorType { kEqual, kUnequal, kGreater, kLess, kGreaterEqual, kLessEqual };

class Operator {
private:
    OperatorType op_;

public:
    Operator(OperatorType type) : op_(type) {}

    std::string ToString() const {
        frozen::string str = OperatorTypeToString.at(op_);
        return std::string(str.begin(), str.end());
    }

    bool operator==(Operator const& rhs) const {
        return op_ == rhs.op_;
    }

    OperatorType GetType() const {
        return op_;
    }

    static constexpr frozen::unordered_map<OperatorType, frozen::string, 6> kOperatorTypeToString{
            {OperatorType::kEqual, "=="},        {OperatorType::kUnequal, "!="},
            {OperatorType::kGreater, ">"},       {OperatorType::kLess, "<"},
            {OperatorType::kGreaterEqual, ">="}, {OperatorType::kLessEqual, "<="}};
};

}  // namespace model
