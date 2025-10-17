#include "config/indices/option.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "common_option.h"
#include "config/exceptions.h"
#include "config/indices/validate_index.h"
#include "config/names_and_descriptions.h"
#include "descriptions.h"
#include "indices/type.h"
#include "names.h"
#include "names_and_descriptions.h"

namespace config {

void IndicesOption::NormalizeIndices(config::IndicesType& indices) {
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
}

IndicesOption::IndicesOption(std::string_view name, std::string_view description,
                             typename Option<config::IndicesType>::NormalizeFunc normalize_func,
                             typename Option<config::IndicesType>::DefaultFunc calculate_default)
    : normalize_(normalize_func != nullptr),
      common_option_(name, description, std::move(calculate_default), std::move(normalize_func),
                     nullptr) {}

std::string_view IndicesOption::GetName() const {
    return common_option_.GetName();
}

Option<config::IndicesType> IndicesOption::operator()(
        config::IndicesType* value_ptr, std::function<config::IndexType()> get_col_count,
        typename Option<config::IndicesType>::ValueCheckFunc value_check_func) const {
    assert(get_col_count);
    Option<config::IndicesType> option = common_option_(value_ptr);
    option.SetValueCheck([get_col_count = std::move(get_col_count),
                          value_check_func = std::move(value_check_func),
                          normalize = normalize_](config::IndicesType const& indices) {
        if (indices.empty()) {
            throw ConfigurationError("Indices cannot be empty");
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
