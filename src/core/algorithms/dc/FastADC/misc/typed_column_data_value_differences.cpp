#include "typed_column_data_value_differences.h"

#include <easylogging++.h>

#include "misc.h"

namespace algos::fastadc {

template <typename T>
static double GetSharedPercentageTyped(model::TypedColumnData const& c1,
                                       model::TypedColumnData const& c2) {
    std::unordered_map<T, size_t> freq_map1;
    std::unordered_map<T, size_t> freq_map2;

    freq_map1.reserve(c1.GetNumRows());
    freq_map2.reserve(c2.GetNumRows());

    for (size_t i = 0; i < c1.GetNumRows(); i++) freq_map1[GetValue<T>(c1, i)]++;
    for (size_t i = 0; i < c2.GetNumRows(); i++) freq_map2[GetValue<T>(c2, i)]++;

    size_t shared_count = 0;
    size_t total_count = 0;

    for (auto const& [data, frequency1] : freq_map1) {
        auto it = freq_map2.find(data);
        size_t frequency2 = (it == freq_map2.end()) ? 0 : it->second;

        shared_count += std::min(frequency1, frequency2);
        total_count += std::max(frequency1, frequency2);
    }

    return total_count > 0 ? static_cast<double>(shared_count) / total_count : 0.0;
}

template <typename T>
static double CalculateAverageTyped(model::TypedColumnData const& column) {
    double sum = 0.0;

    if (column.GetNumRows() == 0) {
        return sum;
    }

    for (size_t i = 0; i < column.GetNumRows(); ++i) {
        sum += static_cast<double>(GetValue<T>(column, i));
    }

    return sum / column.GetNumRows();
}

double GetSharedPercentage(model::TypedColumnData const& c1, model::TypedColumnData const& c2) {
    if (c1.GetColumn() == c2.GetColumn()) return 1.;

    switch (c1.GetTypeId()) {
        case model::TypeId::kInt:
            return GetSharedPercentageTyped<int64_t>(c1, c2);
        case model::TypeId::kDouble:
            return GetSharedPercentageTyped<double>(c1, c2);
        case model::TypeId::kString:
            return GetSharedPercentageTyped<std::string>(c1, c2);
        default:
            LOG(DEBUG) << "Column " << c1.GetColumn()->ToString() << " with type "
                       << c1.GetType().ToString()
                       << " is not supported for shared percentage calculation";
            return -1;
    }
}

double GetAverageRatio(model::TypedColumnData const& c1, model::TypedColumnData const& c2) {
    if (c1.GetColumn() == c2.GetColumn()) return 1.;

    double avg1 = 0.0;
    double avg2 = 0.0;

    switch (c1.GetTypeId()) {
        case model::TypeId::kInt:
            avg1 = CalculateAverageTyped<int64_t>(c1);
            avg2 = CalculateAverageTyped<int64_t>(c2);
            break;
        case model::TypeId::kDouble:
            avg1 = CalculateAverageTyped<double>(c1);
            avg2 = CalculateAverageTyped<double>(c2);
            break;
        default:
            LOG(DEBUG) << "Column type  " << c1.GetType().ToString() << " is not numeric";
            return -1;
    }

    if (avg1 == 0.0 && avg2 == 0.0) {
        return 0.0;
    }

    return std::min(avg1, avg2) / std::max(avg1, avg2);
}

}  // namespace algos::fastadc
