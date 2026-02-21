#pragma once

#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

struct DifFuncInfo {
    std::vector<std::size_t> dif_func_sizes_;
    std::vector<std::size_t> dif_func_nums_;
    std::vector<std::size_t> dif_func_to_column_index_;
    std::vector<std::size_t> dif_func_to_offset_;
    std::vector<boost::dynamic_bitset<>> dif_func_to_bitset_;
    std::size_t dif_func_num_;
    std::size_t num_columns_;
};

}  // namespace algos::dd
