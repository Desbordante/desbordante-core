#include "create_type.h"

#include <string>  // for operator+, string

#include "big_int_type.h"    // for BigIntType
#include "date_type.h"       // for DateType
#include "double_type.h"     // for DoubleType
#include "empty_type.h"      // for EmptyType
#include "int_type.h"        // for IntType
#include "mixed_type.h"      // for MixedType
#include "null_type.h"       // for NullType
#include "string_type.h"     // for StringType
#include "undefined_type.h"  // for UndefinedType

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
        case TypeId::kDate:
            return std::make_unique<DateType>();
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
