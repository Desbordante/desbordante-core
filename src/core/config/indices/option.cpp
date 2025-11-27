#include "core/config/indices/option.h"

#include <algorithm>

#include "core/config/column_index/validate_index.h"
#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"

namespace config {

void IndicesOption::NormalizeIndices(IndicesType& indices) {
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
}

IndicesOption::IndicesOption(std::string_view name, std::string_view description,
                             Option<IndicesType>::NormalizeFunc normalize_func,
                             Option<IndicesType>::DefaultFunc calculate_default, bool allow_empty)
    : normalize_(normalize_func != nullptr),
      common_option_(name, description, std::move(calculate_default), std::move(normalize_func),
                     nullptr),
      allow_empty_(allow_empty) {}

IndicesOption::IndicesOption(std::string_view name, std::string_view description, bool allow_empty)
    : IndicesOption(name, description, NormalizeIndices, nullptr, allow_empty) {}

std::string_view IndicesOption::GetName() const {
    return common_option_.GetName();
}

Option<IndicesType> IndicesOption::operator()(
        IndicesType* value_ptr, std::function<IndexType()> get_col_count,
        Option<IndicesType>::ValueCheckFunc value_check_func) const {
    assert(get_col_count);
    Option<IndicesType> option = common_option_(value_ptr);
    option.SetValueCheck([get_col_count = std::move(get_col_count),
                          value_check_func = std::move(value_check_func), normalize = normalize_,
                          option_name = common_option_.GetName(),
                          allow_empty = allow_empty_](IndicesType const& indices) {
        if (indices.empty()) {
            if (allow_empty) {
                return;
            } else {
                throw ConfigurationError(std::string{option_name} + " cannot be empty");
            }
        }
        static_assert(std::is_unsigned_v<IndexType>);

        auto const get_max_id = [&]() {
            assert(!normalize || std::is_sorted(indices.begin(), indices.end()));
            return normalize ? indices.back() : *std::max_element(indices.begin(), indices.end());
        };

        ValidateIndex(get_max_id(), get_col_count());

        if (value_check_func) value_check_func(indices);
    });
    return option;
}

using names::kLhsIndices, descriptions::kDLhsIndices;
using names::kRhsIndices, descriptions::kDRhsIndices;

extern IndicesOption const kLhsIndicesOpt{kLhsIndices, kDLhsIndices};
extern IndicesOption const kRhsIndicesOpt{kRhsIndices, kDRhsIndices};

extern IndicesOption const kLhsRawIndicesOpt{kLhsIndices, kDLhsIndices, nullptr};
extern IndicesOption const kRhsRawIndicesOpt{kRhsIndices, kDRhsIndices, nullptr};

}  // namespace config
