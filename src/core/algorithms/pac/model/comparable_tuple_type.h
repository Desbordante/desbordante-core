#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "type.h"

namespace pac::model {
using Tuple = std::vector<std::byte const*>;
using Comparer = std::function<bool(Tuple const&, Tuple const&)>;

/// @brief Provides operations to compare fixed-size tuples of values
class ComparableTupleType {
private:
    std::vector<::model::Type const*> types_;
    Comparer comparer_;

public:
    ComparableTupleType(std::vector<::model::Type const*>&& types, Comparer&& comparer)
        : types_(std::move(types)), comparer_(std::move(comparer)) {}

    ComparableTupleType(std::vector<::model::Type const*> const& types, Comparer&& comparer)
        : types_(types), comparer_(std::move(comparer)) {}

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
}  // namespace pac::model
