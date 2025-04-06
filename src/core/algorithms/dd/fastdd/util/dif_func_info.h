#pragma once

#include <cstddef>
#include <vector>

namespace algos::dd {

struct DifFuncInfo {
    std::vector<std::size_t> dif_func_sizes_;
    std::vector<std::size_t> dif_func_nums_;
    std::vector<std::size_t> dif_func_to_node_id_;
    std::vector<std::size_t> dif_func_to_offset_;
    std::size_t dif_func_num_;
    std::size_t num_columns_;
};

}  // namespace algos::dd
