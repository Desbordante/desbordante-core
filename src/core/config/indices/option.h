#pragma once

#include <functional>
#include <string_view>

#include "config/common_option.h"
#include "config/indices/type.h"
#include "indices/option.h"
#include "option.h"

namespace config {

// This class is meant for creating options that are collections of indices.
struct IndicesOption {
    // The normalize function must be set if indexes need to be sorted and deduplicated.
    // For some primitives, the index order is important, so this behavior is configurable.
    IndicesOption(
            std::string_view name, std::string_view description,
            typename Option<config::IndicesType>::NormalizeFunc normalize_func = NormalizeIndices,
            typename Option<config::IndicesType>::DefaultFunc calculate_default = nullptr);

    static void NormalizeIndices(config::IndicesType& indices);

    [[nodiscard]] std::string_view GetName() const;

    // These options always check that no indices are out of bounds, and may
    // sometimes check other things as well.
    [[nodiscard]] Option<config::IndicesType> operator()(
            config::IndicesType* value_ptr, std::function<config::IndexType()> get_col_count,
            typename Option<config::IndicesType>::ValueCheckFunc value_check_func = nullptr) const;

private:
    bool normalize_;
    CommonOption<config::IndicesType> const common_option_;
};

extern IndicesOption const kLhsIndicesOpt;
extern IndicesOption const kRhsIndicesOpt;

extern IndicesOption const kLhsRawIndicesOpt;
extern IndicesOption const kRhsRawIndicesOpt;

}  // namespace config
