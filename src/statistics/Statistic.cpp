#include "Statistic.h"
#include "Types.h"

namespace statistics {

Statistic::Statistic() noexcept: hasValue_(false), data(nullptr), type(nullptr) {}

Statistic::Statistic(std::byte const* data, model::Type const* type) noexcept: 
    hasValue_(true), data(const_cast<std::byte*>(data)), 
    type(const_cast<model::Type *>(type)) {}

void Statistic::Free() {
    if(hasValue_) {
        type->Free(data);
        delete type;
        hasValue_ = false;
    }
}

bool Statistic::hasValue() const noexcept{
    return hasValue_;
}

std::byte const* Statistic::getDataAndFree() {
    const auto res = data;
    data = nullptr;
    delete type;
    hasValue_ = false;
    return res;
}

std::byte const* Statistic::getData() {
    return data;
}

model::Type const* Statistic::getType(){
    return type;
}

std::string Statistic::toString() {
    if(!hasValue_)
        return "";
    return type->ValueToString(data);
}

model::Type* Statistic::getTypeClone(const model::Type& type, bool 
    is_null_eq_null) {
    switch (type.GetTypeId())
    {
    case model::TypeId::kInt:
        return new model::IntType();
    case model::TypeId::kMixed:
        return new model::MixedType(is_null_eq_null);
    case model::TypeId::kString:
        return new model::StringType();
    case model::TypeId::kDouble:
        return new model::DoubleType();
    case model::TypeId::kBigInt:
        return new model::BigIntType();
    case model::TypeId::kEmpty:
        return new model::EmptyType();
    case model::TypeId::kUndefined:
        return new model::UndefinedType(is_null_eq_null);
    default:
        return new model::IntType();
    }
}

void ColumnStats::Free() {
    avg.Free();
    STD.Free();
    skewness.Free();
    kurtosis.Free();
    min.Free();
    max.Free();
    sum.Free();
    quantile25.Free();
    quantile50.Free();
    quantile75.Free();
}

std::string ColumnStats::toString() {
    std::stringstream res;
    res << "avg = " << avg.toString() << std::endl;
    res << "STD = " << STD.toString() << std::endl;
    res << "skewness = " << skewness.toString() << std::endl;
    res << "kurtosis = " << kurtosis.toString() << std::endl;
    res << "min = " << min.toString() << std::endl;
    res << "max = " << max.toString() << std::endl;
    res << "sum = " << sum.toString() << std::endl;
    res << "quantile25 = " << quantile25.toString() << std::endl;
    res << "quantile50 = " << quantile50.toString() << std::endl;
    res << "quantile75 = " << quantile75.toString() << std::endl;
    res << "count = " << count << std::endl;
    res << "distinct = " << distinct << std::endl;
    res << "isCategorical = " << (isCategorical ? "true" : "false") << std::endl;
    return res.str();
}

}