#pragma once

#include <magic_enum/magic_enum.hpp>

#include "core/util/export.h"

namespace algos {

enum class DESBORDANTE_EXPORT Binop : char {
    kAddition = '+',
    kSubtraction = '-',
    kMultiplication = '*',
    kDivision = '/'
};
}  // namespace algos
