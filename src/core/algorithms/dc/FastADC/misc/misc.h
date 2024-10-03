#pragma once

#include "model/table/typed_column_data.h"

namespace algos::fastadc {

namespace {
// Helper to trigger a compile-time error for unsupported types
template <typename T>
struct DependentFalse : std::false_type {};
}  // namespace

template <typename T>
[[nodiscard]] inline T const& GetValue(model::TypedColumnData const& column, size_t row) {
    model::Type const& type = column.GetType();

    if (!column.IsNullOrEmpty(row)) {
        return type.GetValue<T>(column.GetValue(row));
    }

    /*
     * Mimicking the Java behavior:
     * https://github.com/RangerShaw/FastADC/blob/master/src/main/java/de/metanome/algorithms/dcfinder/input/Column.java#L71
     *
     * public Long getLong(int line) {
     *     return values.get(line).isEmpty() ? Long.MIN_VALUE :
     *             Long.parseLong(values.get(line));
     * }
     *
     * public Double getDouble(int line) {
     *     return values.get(line).isEmpty() ? Double.MIN_VALUE :
     *            Double.parseDouble(values.get(line));
     * }
     *
     * public String getString(int line) {
     *     return values.get(line) == null ? "" : values.get(line);
     * }
     */
    if constexpr (std::is_same_v<T, std::string>) {
        static std::string const kEmptyStr = "";
        return kEmptyStr;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        static int64_t const kMinInt = std::numeric_limits<int64_t>::min();
        return kMinInt;
    } else if constexpr (std::is_same_v<T, double>) {
        static double const kMinDouble = std::numeric_limits<double>::lowest();
        return kMinDouble;
    } else {
        static_assert(DependentFalse<T>::value,
                      "FastADC algorithm supports only int64_t, string, or double as column types. "
                      "This function should not be called with other types.");
    }
}

}  // namespace algos::fastadc
