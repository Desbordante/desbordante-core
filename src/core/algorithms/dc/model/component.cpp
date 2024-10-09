#include "component.h"

#include <string>

#include "operator.h"
#include "types.h"

namespace algos {

namespace dc {

namespace mo = model;

bool Component::CompareNumeric(std::byte const* l_val, mo::Type const* lhs_type,
                               std::byte const* r_val, mo::Type const* rhs_type,
                               mo::CompareResult res) const {
    auto l_type = dynamic_cast<mo::INumericType const*>(lhs_type);
    auto r_type = dynamic_cast<mo::INumericType const*>(rhs_type);
    return l_type->CompareNumeric(l_val, l_type, r_val, r_type) == res;
}

std::string Component::ToString() const {
    if (val_type_ == ValType::kPlusInf) return "+Inf";
    if (val_type_ == ValType::kMinusInf) return "-Inf";

    mo::TypeId type_id = type_->GetTypeId();
    switch (type_id) {
        case mo::TypeId::kInt:
            return std::to_string(mo::Type::GetValue<mo::Int>(val_));
            break;
        case mo::TypeId::kDouble:
            return std::to_string(mo::Type::GetValue<mo::Double>(val_));
            break;
        case mo::TypeId::kString:
            return mo::Type::GetValue<mo::String>(val_);
            break;
        default:
            assert(false);
            __builtin_unreachable();
    }
}

bool Component::operator<(Component const& rhs) const {
    assert(type_->IsMetrizable() and rhs.type_->IsMetrizable());
    assert(type_->IsNumeric() == rhs.type_->IsNumeric());  // both or neither

    if (val_type_ < rhs.val_type_) return true;
    if (val_type_ > rhs.val_type_) return false;
    if (val_type_ != ValType::kFinite) return true;

    if (type_->IsNumeric())
        return CompareNumeric(val_, type_, rhs.val_, rhs.type_, mo::CompareResult::kLess);

    return type_->Compare(val_, rhs.val_) == mo::CompareResult::kLess;
}

bool Component::operator==(Component const& rhs) const {
    assert(type_->IsMetrizable() and rhs.type_->IsMetrizable());
    assert(type_->IsNumeric() == rhs.type_->IsNumeric());  // both or neither

    if (val_type_ != rhs.val_type_) return false;
    if (val_type_ != ValType::kFinite) return true;

    if (type_->IsNumeric())
        return CompareNumeric(val_, type_, rhs.val_, rhs.type_, mo::CompareResult::kEqual);

    return type_->Compare(val_, rhs.val_) == mo::CompareResult::kEqual;
}

bool Component::Eval(Component const& lhs, Component const& rhs, Operator const& op) {
    switch (op.GetType()) {
        case OperatorType::kLess:
            return lhs < rhs;
        case OperatorType::kLessEqual:
            return lhs <= rhs;
        case OperatorType::kGreater:
            return lhs > rhs;
        case OperatorType::kGreaterEqual:
            return lhs >= rhs;
        case OperatorType::kEqual:
            return lhs == rhs;
        case OperatorType::kUnequal:
            return lhs != rhs;
        default:
            assert(false);
            __builtin_unreachable();
    };
}

}  // namespace dc

}  // namespace algos
