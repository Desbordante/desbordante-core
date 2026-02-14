#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/model/types/builtin.h"
#include "core/model/types/type.h"

namespace pac::model {
/// @brief Provides operations for fixed-size tuples of values on a fixed set of attributes
class TupleType {
private:
    std::vector<::model::Type const*> types_;

public:
    TupleType(std::vector<::model::Type const*>&& types) : types_(std::move(types)) {}

    /// @brief Convert @c std::byte* of type @c types[type_num] to string.
    /// Consider using this function rather than directly calling @c types[type_num]->ValueToString,
    /// because it correctly handles NULLs
    std::string ByteToString(std::size_t type_num, std::byte const* value) const {
        assert(type_num < types_.size());

        if (value == nullptr) {
            return "NULL";
        }
        return types_[type_num]->ValueToString(value);
    }

    /// @brief Compare two @c std::byte pointers of type @c types[type_num].
    /// Consider using this function rather than directly call @c types[type_num]->Compare, because
    /// it correctly handles NULLs (NULL is considered less than any value).
    ::model::CompareResult CompareBytes(std::size_t const type_num, std::byte const* x,
                                        std::byte const* y);

    std::string ValueToString(Tuple const& value) const;

    std::vector<::model::Type const*> const& GetTypes() const {
        return types_;
    }
};

/// @brief Converts vector of strings of the same format as ComparableTupleType::ValueToString()
/// does
std::string StringValueToString(std::vector<std::string> const& strings);
}  // namespace pac::model
