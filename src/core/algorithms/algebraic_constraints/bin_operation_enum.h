#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos {

enum class Binop : char {
    kAddition = '+',
    kSubtraction = '-',
    kMultiplication = '*',
    kDivision = '/'
};
}  // namespace algos
