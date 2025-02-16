#include "dc/FastADC/model/operator.h"

#include "model/types/builtin.h"
#include "model/types/type.h"

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

}  // namespace algos::fastadc
