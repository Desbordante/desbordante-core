#include "operator.h"

#include <utility>

#include "builtin.h"
#include "type.h"

namespace algos::fastadc {

bool Operator::Eval(std::byte const* v1, std::byte const* v2, model::Type const& type) const {
    model::CompareResult cr = type.Compare(v1, v2);

    switch (op_) {
        case OperatorType::kEqual:
            return cr == model::CompareResult::kEqual;
        case OperatorType::kUnequal:
            return cr != model::CompareResult::kEqual;
        case OperatorType::kGreater:
            return cr == model::CompareResult::kGreater;
        case OperatorType::kLess:
            return cr == model::CompareResult::kLess;
        case OperatorType::kGreaterEqual:
            return cr == model::CompareResult::kGreater || cr == model::CompareResult::kEqual;
        case OperatorType::kLessEqual:
            return cr == model::CompareResult::kLess || cr == model::CompareResult::kEqual;
        default:
            return false;
    }
}

Operator::OperatorMap<OperatorType> Operator::InitializeInverseMap() {
    return {{OperatorType::kEqual, OperatorType::kUnequal},
            {OperatorType::kUnequal, OperatorType::kEqual},
            {OperatorType::kGreater, OperatorType::kLessEqual},
            {OperatorType::kLess, OperatorType::kGreaterEqual},
            {OperatorType::kGreaterEqual, OperatorType::kLess},
            {OperatorType::kLessEqual, OperatorType::kGreater}};
}

Operator::OperatorMap<OperatorType> Operator::InitializeSymmetricMap() {
    return {{OperatorType::kEqual, OperatorType::kEqual},
            {OperatorType::kUnequal, OperatorType::kUnequal},
            {OperatorType::kGreater, OperatorType::kLess},
            {OperatorType::kLess, OperatorType::kGreater},
            {OperatorType::kGreaterEqual, OperatorType::kLessEqual},
            {OperatorType::kLessEqual, OperatorType::kGreaterEqual}};
}

Operator::OperatorMap<std::vector<OperatorType>> Operator::InitializeImplicationsMap() {
    return {{OperatorType::kEqual,
             {OperatorType::kEqual, OperatorType::kGreaterEqual, OperatorType::kLessEqual}},
            {OperatorType::kUnequal, {OperatorType::kUnequal}},
            {OperatorType::kGreater,
             {OperatorType::kGreater, OperatorType::kGreaterEqual, OperatorType::kUnequal}},
            {OperatorType::kLess,
             {OperatorType::kLess, OperatorType::kLessEqual, OperatorType::kUnequal}},
            {OperatorType::kGreaterEqual, {OperatorType::kGreaterEqual}},
            {OperatorType::kLessEqual, {OperatorType::kLessEqual}}};
}

Operator::OperatorMap<std::vector<OperatorType>> Operator::InitializeTransitivesMap() {
    return {{OperatorType::kEqual, {OperatorType::kEqual}},
            {OperatorType::kUnequal, {OperatorType::kEqual}},
            {OperatorType::kGreater,
             {OperatorType::kGreater, OperatorType::kGreaterEqual, OperatorType::kEqual}},
            {OperatorType::kLess,
             {OperatorType::kLess, OperatorType::kLessEqual, OperatorType::kEqual}},
            {OperatorType::kGreaterEqual,
             {OperatorType::kGreater, OperatorType::kGreaterEqual, OperatorType::kEqual}},
            {OperatorType::kLessEqual,
             {OperatorType::kLess, OperatorType::kLessEqual, OperatorType::kEqual}}};
}

Operator::OperatorMap<std::string> Operator::InitializeShortStringMap() {
    return {{OperatorType::kEqual, "=="},        {OperatorType::kUnequal, "!="},
            {OperatorType::kGreater, ">"},       {OperatorType::kLess, "<"},
            {OperatorType::kGreaterEqual, ">="}, {OperatorType::kLessEqual, "<="}};
}

Operator::OperatorMap<OperatorType> const Operator::kInverseMap = InitializeInverseMap();
Operator::OperatorMap<OperatorType> const Operator::kSymmetricMap = InitializeSymmetricMap();
Operator::OperatorMap<std::vector<OperatorType>> const Operator::kImplicationsMap =
        InitializeImplicationsMap();
Operator::OperatorMap<std::vector<OperatorType>> const Operator::kTransitivesMap =
        InitializeTransitivesMap();
Operator::OperatorMap<std::string> const Operator::kShortStringMap = InitializeShortStringMap();

}  // namespace algos::fastadc
