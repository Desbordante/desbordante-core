#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>

#include <boost/pointer_cast.hpp>

#include "builtin.h"
#include "type.h"

namespace model {

std::unique_ptr<Type> CreateType(TypeId const type_id, bool const is_null_eq_null);

template <typename T>
std::unique_ptr<T> CreateSpecificType(TypeId const type_id, bool const is_null_eq_null) {
    static_assert(std::is_base_of_v<Type, T>, "T should be derived from Type!");
    auto specific_type = boost::dynamic_pointer_cast<T>(CreateType(type_id, is_null_eq_null));
    if (specific_type == nullptr) {
        throw std::invalid_argument("Specified type_id is not correct for type T");
    }
    return specific_type;
}

}  // namespace model
