#include "type_util.h"

#include <compare>
#include <string>

#include "builtin.h"

namespace algos::fastod {

bool IsUnorderedType(model::TypeId type_id) {
    return type_id == +model::TypeId::kEmpty || type_id == +model::TypeId::kNull ||
           type_id == +model::TypeId::kUndefined;
}

model::CompareResult CompareDataAsStrings(std::byte const* left, std::byte const* right,
                                          model::MixedType const* mixed_type) {
    std::string const left_str = mixed_type->ValueToString(left);
    std::string const right_str = mixed_type->ValueToString(right);

    if (left_str == right_str) {
        return model::CompareResult::kEqual;
    }

    if (left_str < right_str) {
        return model::CompareResult::kLess;
    }

    return model::CompareResult::kGreater;
}

}  // namespace algos::fastod
