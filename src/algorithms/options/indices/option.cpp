#include "algorithms/options/indices/option.h"

#include <algorithm>
#include <stdexcept>

#include "algorithms/options/indices/validate_index.h"
#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {

static void NormalizeIndices(config::IndicesType& indices) {
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
}

IndicesOption::IndicesOption(std::string_view name, std::string_view description)
    : common_option_(name, description, {}, NormalizeIndices) {}

std::string_view IndicesOption::GetName() const {
    return common_option_.GetName();
}

Option<config::IndicesType> IndicesOption::operator()(
        config::IndicesType* value_ptr, std::function<config::IndexType()> get_col_count,
        typename Option<config::IndicesType>::ValueCheckFunc value_check_func_) const {
    assert(get_col_count);
    Option<config::IndicesType> option = common_option_(value_ptr);
    option.SetValueCheck(
            [get_col_count = std::move(get_col_count),
             value_check_func_ = std::move(value_check_func_)](config::IndicesType const& indices) {
                if (indices.empty()) {
                    throw std::invalid_argument("Indices cannot be empty");
                }
                static_assert(std::is_unsigned_v<config::IndexType>);
                assert(std::is_sorted(indices.begin(), indices.end()));
                config::ValidateIndex(indices.back(), get_col_count());
                if (value_check_func_) value_check_func_(indices);
            });
    return option;
}

using config::names::kLhsIndices, config::descriptions::kDLhsIndices;
using config::names::kRhsIndices, config::descriptions::kDRhsIndices;
extern const IndicesOption LhsIndicesOpt{kLhsIndices, kDLhsIndices};
extern const IndicesOption RhsIndicesOpt{kRhsIndices, kDRhsIndices};

}  // namespace algos::config
