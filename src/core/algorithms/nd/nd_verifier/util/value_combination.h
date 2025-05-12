#pragma once

#include <cstddef>  // for byte, size_t
#include <ostream>  // for ostream
#include <string>   // for string
#include <utility>  // for pair
#include <vector>   // for vector

#include "model/types/builtin.h"  // for TypeId

namespace algos::nd_verifier::util {

class ValueCombination {
private:
    using TypedValue = std::pair<model::TypeId, std::byte const*>;

    std::vector<TypedValue> typed_data_;

    bool CompareValues(TypedValue a, TypedValue b);

public:
    ValueCombination(std::vector<TypedValue> typed_data) : typed_data_(typed_data) {}

    std::string ToString() const;

    bool operator==(ValueCombination const& other) {
        return !(*this != other);
    }

    bool operator!=(ValueCombination const& other) {
        if (typed_data_.size() != other.typed_data_.size()) {
            return true;
        }

        for (size_t i{0}; i < typed_data_.size(); ++i) {
            auto first_pair = typed_data_[i];
            auto second_pair = other.typed_data_[i];

            if (!CompareValues(first_pair, second_pair)) {
                return true;
            }
        }

        return false;
    }
};

std::ostream& operator<<(std::ostream& os, ValueCombination const& vc);

}  // namespace algos::nd_verifier::util
