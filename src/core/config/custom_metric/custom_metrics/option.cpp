#include "core/config/custom_metric/custom_metrics/option.h"

#include <cassert>
#include <functional>

#include "core/config/column_index/type.h"
#include "core/config/option.h"

namespace config {
Option<CustomMetricsType> MetricsOption::operator()(
        CustomMetricsType* value_ptr, std::function<IndexType()> get_col_count) const {
    assert(get_col_count);
    auto option = common_option_(value_ptr);
    option.SetValueCheck([get_col_count](CustomMetricsType const& value) {
        return CheckMetrics(value, get_col_count());
    });
    option.SetNormalizeFunc([get_col_count](CustomMetricsType& value) {
        return NormalizeMetrics(value, get_col_count());
    });
    return option;
}
}  // namespace config
