#pragma once

#include <memory>

#include "numeric_type.h"
#include "type_wrapper.h"

namespace algos {

struct TypedColumnPair {
    /* Column indexes, the first for the column whose values were the left operand
     * for binop_, the second for the right */
    std::pair<size_t, size_t> col_i;
    /* Columns type */
    TypeWrapper type_wrapper;
};

}  // namespace algos
