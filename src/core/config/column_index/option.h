#pragma once

#include <string_view>

#include "core/config/column_index/type.h"
#include "core/config/common_option.h"

namespace config {

// Simplifies creating of options that represent a single index of the attribute in the table
class ColumnIndexOption {
public:
    ColumnIndexOption(std::string_view name, std::string_view description,
                      Option<IndexType>::DefaultFunc get_default_value = nullptr);

    [[nodiscard]] std::string_view GetName() const;
    // Creates an option which performs a check that index is not greater than column count
    [[nodiscard]] Option<IndexType> operator()(
            IndexType* value_ptr, std::function<IndexType()> get_col_count,
            Option<IndexType>::ValueCheckFunc value_check_func = nullptr) const;

private:
    CommonOption<IndexType> const common_option_;
};

}  // namespace config
