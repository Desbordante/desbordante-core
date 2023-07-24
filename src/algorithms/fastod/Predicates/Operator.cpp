#include <string>

#include "Operator.h"
#include "SchemaValue.h"
#include "OperatorType.h"

using namespace algos::fastod;

Operator::Operator(OperatorType type) noexcept : type_(type) { }

OperatorType Operator::GetType() const noexcept {
    return type_;
}

int Operator::GetTypeAsInt() const noexcept {
    return static_cast<int>(type_);
}

std::string Operator::ToString() const noexcept {
    switch (type_){
        case OperatorType::Equal: return "=";
        case OperatorType::Less: return "<";
        case OperatorType::Greater: return ">";
        case OperatorType::LessOrEqual: return "<=";
        case OperatorType::GreaterOrEqual: return ">=";
        case OperatorType::NotEqual: return "!=";

        default: return "*";
    }
}

Operator Operator::Reverse() const noexcept {
    switch (type_) {
        case OperatorType::Equal: return Operator(OperatorType::NotEqual);
        case OperatorType::Less: return Operator(OperatorType::GreaterOrEqual);
        case OperatorType::Greater: return Operator(OperatorType::LessOrEqual);
        case OperatorType::LessOrEqual: return Operator(OperatorType::Greater);
        case OperatorType::GreaterOrEqual: return Operator(OperatorType::Less);
        case OperatorType::NotEqual: return Operator(OperatorType::Equal);
    }
}

bool Operator::Oppose(Operator const& other) const noexcept {
    OperatorType otherType = other.GetType();
    
    switch (type_) {
        case OperatorType::Equal:
            if (otherType == OperatorType::Less
                || otherType == OperatorType::Greater
                || otherType == OperatorType::NotEqual) {
                return true;
            }
            break;
        case OperatorType::Less:
            if (otherType == OperatorType::GreaterOrEqual
                || otherType == OperatorType::Equal
                || otherType == OperatorType::Greater) {
                return true;
            }
            break;
        case OperatorType::Greater:
            if (otherType == OperatorType::LessOrEqual
                || otherType == OperatorType::Equal
                || otherType == OperatorType::Less) {
                return true;
            }
            break;
        case OperatorType::LessOrEqual:
            if (otherType == OperatorType::Greater)
                return true;
            break;
        case OperatorType::GreaterOrEqual:
            if (otherType == OperatorType::Less)
                return true;
            break;
        case OperatorType::NotEqual:
            if (otherType == OperatorType::Equal)
                return true;
            break;
            
        default: return false;
    }
}

bool Operator::Imply(Operator const& other) const noexcept {
    if (*this == other)
        return true;

    OperatorType otherType = other.GetType();
    
    switch (type_) {
        case OperatorType::Equal:
            if (otherType == OperatorType::LessOrEqual
                || otherType == OperatorType::GreaterOrEqual) {
                return true;
            }
            break;
        case OperatorType::Less:
            if (otherType == OperatorType::LessOrEqual
                || otherType == OperatorType::NotEqual) {
                return true;
            }
            break;
        case OperatorType::Greater:
            if (otherType == OperatorType::LessOrEqual
                || otherType == OperatorType::NotEqual) {
                return true;
            }
            break;
    }

    return false;
}

bool Operator::Violate(SchemaValue const& first, SchemaValue const& second) const noexcept {
    switch (type_) {
        case OperatorType::Equal: return first != second;
        case OperatorType::Less: return first >= second;
        case OperatorType::Greater: return first <= second;
        case OperatorType::LessOrEqual: return first > second;
        case OperatorType::GreaterOrEqual: return first < second;
        case OperatorType::NotEqual: return first == second;

        default: return false;
    }
}

bool Operator::IsLessOrGreater() const noexcept {
    return type_ == OperatorType::Less
        || type_ == OperatorType::Greater
        || type_ == OperatorType::GreaterOrEqual
        || type_ == OperatorType::LessOrEqual;
}

std::vector<Operator> Operator::SupportedOperators() noexcept {
    return std::vector<Operator> {
        Operator(OperatorType::Equal),
        Operator(OperatorType::Less),
        Operator(OperatorType::Greater),
        Operator(OperatorType::LessOrEqual),
        Operator(OperatorType::GreaterOrEqual),
        Operator(OperatorType::NotEqual)
    };
}

bool algos::fastod::operator==(Operator const& x, Operator const& y) {
    return x.GetType() == y.GetType();
}

bool algos::fastod::operator!=(Operator const& x, Operator const& y) {
    return !(x == y);
}
