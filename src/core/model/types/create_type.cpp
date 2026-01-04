#include "core/model/types/create_type.h"

#include "core/model/types/big_int_type.h"
#include "core/model/types/date_type.h"
#include "core/model/types/double_type.h"
#include "core/model/types/empty_type.h"
#include "core/model/types/int_type.h"
#include "core/model/types/mixed_type.h"
#include "core/model/types/null_type.h"
#include "core/model/types/numeric_type.h"
#include "core/model/types/string_type.h"
#include "core/model/types/undefined_type.h"

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
