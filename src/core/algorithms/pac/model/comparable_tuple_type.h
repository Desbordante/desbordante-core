#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/pac/model/tuple.h"
#include "builtin.h"
#include "model/types/type.h"

namespace pac::model {
using Comparer = std::function<bool(Tuple const&, Tuple const&)>;

/// @brief Provides operations to compare fixed-sized tuples of values
class ComparableTupleType {
private:
    std::vector<::model::Type const*> types_;
    Comparer comparer_;

public:
    ComparableTupleType(std::vector<::model::Type const*>&& types, Comparer&& comparer)
        : types_(std::move(types)), comparer_(std::move(comparer)) {}

    /// @brief Convert @c std::byte* of type @c types[type_num] to string.
    /// Consider using this function rather than directly calling @c types[type_num]->ValueToString,
    /// because it correctly handles NULLs
    std::string ByteToString(std::size_t type_num, std::byte const* value) const {
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

    /// @brief Comapre two value tuples
    /// @note This function is not guaranteed to define total strict order relation.
    /// Do not use it as a comparer where such relation is needed (e. g. std::set)
    bool Less(Tuple const& x, Tuple const& y) const {
        assert(x.size() >= types_.size());
        assert(y.size() >= types_.size());

        return comparer_(x, y);
    }

    std::string ValueToString(Tuple const& value) const;

    std::vector<::model::Type const*> const& GetTypes() const {
        return types_;
    }
};

/// @brief Converts vector of strings of the same format as ComparableTupleType::ValueToString()
/// does
std::string StringValueToString(std::vector<std::string> const& strings);
}  // namespace pac::model
