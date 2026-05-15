#pragma once

#include <cstddef>
#include <vector>

#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
struct DifFuncInfo {
    std::vector<std::size_t> dif_func_sizes_;
    std::vector<std::size_t> dif_func_nums_;
    std::vector<std::size_t> dif_func_to_column_index_;
    std::vector<std::size_t> dif_func_to_offset_;
    std::vector<Bitset> dif_func_to_bitset_;
    std::size_t dif_func_num_;
    std::size_t num_columns_;
};

}  // namespace algos::dd
