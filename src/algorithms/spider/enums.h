#pragma once

#include <enum.h>

namespace algos::ind {

BETTER_ENUM(KeyType, char, STRING_VIEW, PAIR)
BETTER_ENUM(ColType, char, SET, VECTOR)

namespace details {
enum class KeyTypeImpl { STRING_VIEW, PAIR };
enum class ColTypeImpl { SET, VECTOR };
}  // namespace details

};  // namespace algos::ind
