#include "core/algorithms/dc/FastADC/model/column_operand.h"

namespace algos::fastadc {

size_t hash_value(ColumnOperand const& k) noexcept {
    size_t seed = 0;
    boost::hash_combine(seed, k.GetColumn()->GetIndex());
    boost::hash_combine(seed, k.GetTuple()._value);
    return seed;
}

}  // namespace algos::fastadc
