#include "column_operand.h"

namespace algos::fastadc {

size_t hash_value(ColumnOperand const& col_op) noexcept {
    return std::hash<ColumnOperand>()(col_op);
}

}  // namespace algos::fastadc
