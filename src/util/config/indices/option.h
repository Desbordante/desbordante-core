#pragma once

#include <vector>

#include "util/config/common_option.h"
#include "util/config/indices/type.h"

namespace util::config {

// This class is meant for creating options that are collections of indices.
struct IndicesOption {
    IndicesOption(std::string_view name, std::string_view description);

    [[nodiscard]] std::string_view GetName() const;

    // These options always check that no indices are out of bounds, and may
    // sometimes check other things as well.
    [[nodiscard]] Option<config::IndicesType> operator()(
            config::IndicesType* value_ptr, std::function<config::IndexType()> get_col_count,
            typename Option<config::IndicesType>::ValueCheckFunc value_check_func_ = nullptr) const;

private:
    CommonOption<config::IndicesType> const common_option_;
};

extern const IndicesOption LhsIndicesOpt;
extern const IndicesOption RhsIndicesOpt;

}  // namespace util::config
