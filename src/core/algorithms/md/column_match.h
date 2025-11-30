#pragma once

#include <cstddef>
#include <string>

#include "core/model/index.h"

namespace model::md {

struct ColumnMatch {
    Index left_col_index;
    Index right_col_index;
    std::string name;

    ColumnMatch(Index left_col_index, Index right_col_index,
                std::string similarity_function_name) noexcept
        : left_col_index(left_col_index),
          right_col_index(right_col_index),
          name(std::move(similarity_function_name)) {}
};

}  // namespace model::md
