#include "config/indices/option.h"

#include <algorithm>

#include "config/column_index/validate_index.h"
#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {

void IndicesOption::NormalizeIndices(config::IndicesType& indices) {
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
}

IndicesOption::IndicesOption(std::string_view name, std::string_view description,
                             typename Option<config::IndicesType>::NormalizeFunc normalize_func,
                             typename Option<config::IndicesType>::DefaultFunc calculate_default,
                             bool allow_empty)
    : normalize_(normalize_func != nullptr),
      common_option_(name, description, std::move(calculate_default), std::move(normalize_func),
                     nullptr),
      allow_empty_(allow_empty) {}

IndicesOption::IndicesOption(std::string_view name, std::string_view description, bool allow_empty)
    : IndicesOption(name, description, NormalizeIndices, nullptr, allow_empty) {}

std::string_view IndicesOption::GetName() const {
    return common_option_.GetName();
}

Option<config::IndicesType> IndicesOption::operator()(
        config::IndicesType* value_ptr, std::function<config::IndexType()> get_col_count,
        typename Option<config::IndicesType>::ValueCheckFunc value_check_func) const {
    assert(get_col_count);
    Option<config::IndicesType> option = common_option_(value_ptr);
    option.SetValueCheck([get_col_count = std::move(get_col_count),
                          value_check_func = std::move(value_check_func), normalize = normalize_,
                          option_name = common_option_.GetName(),
                          allow_empty = allow_empty_](config::IndicesType const& indices) {
        if (!allow_empty && indices.empty()) {
            throw ConfigurationError(std::string{option_name} + " cannot be empty");
        }
        static_assert(std::is_unsigned_v<config::IndexType>);

        auto const get_max_id = [&]() {
            assert(!normalize || std::is_sorted(indices.begin(), indices.end()));
            return normalize ? indices.back() : *std::max_element(indices.begin(), indices.end());
        };

        config::ValidateIndex(get_max_id(), get_col_count());

        if (value_check_func) value_check_func(indices);
    });
    return option;
}

using config::names::kLhsIndices, config::descriptions::kDLhsIndices;
using config::names::kRhsIndices, config::descriptions::kDRhsIndices;

extern IndicesOption const kLhsIndicesOpt{kLhsIndices, kDLhsIndices};
extern IndicesOption const kRhsIndicesOpt{kRhsIndices, kDRhsIndices};

extern IndicesOption const kLhsRawIndicesOpt{kLhsIndices, kDLhsIndices, nullptr};
extern IndicesOption const kRhsRawIndicesOpt{kRhsIndices, kDRhsIndices, nullptr};

}  // namespace config
