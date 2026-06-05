#include "core/algorithms/dc/FastADC/model/column_operand.h"

#include <magic_enum/magic_enum.hpp>

namespace algos::fastadc {

size_t hash_value(ColumnOperand const& k) noexcept {
    size_t seed = 0;
    boost::hash_combine(seed, k.GetColumn()->GetIndex());
    boost::hash_combine(seed, magic_enum::enum_integer(k.GetTuple()));
    return seed;
}

}  // namespace algos::fastadc
