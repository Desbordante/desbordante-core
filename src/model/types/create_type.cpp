#include "create_type.h"

#include "big_int_type.h"
#include "double_type.h"
#include "empty_type.h"
#include "int_type.h"
#include "mixed_type.h"
#include "null_type.h"
#include "numeric_type.h"
#include "string_type.h"
#include "undefined_type.h"

namespace model {

std::unique_ptr<Type> CreateType(TypeId const type_id, bool const is_null_eq_null) {
    switch (type_id) {
    case TypeId::kInt:
        return std::make_unique<IntType>();
    case TypeId::kDouble:
        return std::make_unique<DoubleType>();
    case TypeId::kString:
        return std::make_unique<StringType>();
    case TypeId::kBigInt:
        return std::make_unique<BigIntType>();
    case TypeId::kNull:
        return std::make_unique<NullType>(is_null_eq_null);
    case TypeId::kEmpty:
        return std::make_unique<EmptyType>();
    case TypeId::kMixed:
        return std::make_unique<MixedType>(is_null_eq_null);
    case TypeId::kUndefined: {
        std::unique_ptr<NullType> undefined = std::make_unique<UndefinedType>(is_null_eq_null);
        return undefined;
    }
    default:
        throw std::invalid_argument(std::string("Invalid type_id in function: ") + __func__);
    }
}

}  // namespace model
