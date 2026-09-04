#include "core/config/custom_metric/custom_metrics/option.h"

#include <cassert>
#include <functional>

#include "core/config/column_index/type.h"
#include "core/config/option.h"

namespace config {
Option<CustomMetricsOptionType> MetricsOption::operator()(
        CustomMetricsOptionType* value_ptr, std::function<IndexType()> get_col_count) const {
    assert(get_col_count);
    auto make_default = [get_col_count]() { return MakeDefaultMetrics(get_col_count()); };
    auto option = Option<CustomMetricsOptionType>(value_ptr, name_, description_, make_default);
    option.SetValueCheck([get_col_count](CustomMetricsOptionType const& value) {
        return CheckMetrics(value, get_col_count());
    });
    option.SetNormalizeFunc(NormalizeMetrics);
    return option;
}
}  // namespace config
