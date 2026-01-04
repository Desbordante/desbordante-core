#pragma once

#include <vector>

#include "core/config/common_option.h"
#include "core/config/indices/type.h"

namespace config {

// This class is meant for creating options that are collections of indices.
struct IndicesOption {
    // The normalize function must be set if indexes need to be sorted and deduplicated.
    // For some primitives, the index order is important, so this behavior is configurable.
    IndicesOption(std::string_view name, std::string_view description,
                  Option<IndicesType>::NormalizeFunc normalize_func = NormalizeIndices,
                  Option<IndicesType>::DefaultFunc calculate_default = nullptr,
                  bool allow_empty = false);
    IndicesOption(std::string_view name, std::string_view description, bool allow_empty);

    static void NormalizeIndices(IndicesType& indices);

    [[nodiscard]] std::string_view GetName() const;

    // These options always check that no indices are out of bounds, and may
    // sometimes check other things as well.
    [[nodiscard]] Option<IndicesType> operator()(
            IndicesType* value_ptr, std::function<IndexType()> get_col_count,
            Option<IndicesType>::ValueCheckFunc value_check_func = nullptr) const;

private:
    bool normalize_;
    CommonOption<IndicesType> const common_option_;
    bool allow_empty_;
};

extern IndicesOption const kLhsIndicesOpt;
extern IndicesOption const kRhsIndicesOpt;

extern IndicesOption const kLhsRawIndicesOpt;
extern IndicesOption const kRhsRawIndicesOpt;

}  // namespace config
