#include "value_range.h"

namespace model {

template <typename T, typename RangeT>
RangeT FindRangeOf(model::TypedColumnData const& column) {
    bool initialized = false;
    T lower_bound;
    T upper_bound;
    for (size_t row_index = 0; row_index < column.GetNumRows(); ++row_index) {
        std::byte const* value = column.GetValue(row_index);
        T encountered_value = model::Type::GetValue<T>(value);
        if (!initialized) {
            lower_bound = encountered_value;
            upper_bound = encountered_value;
            initialized = true;
            continue;
        }
        if (encountered_value > upper_bound) {
            upper_bound = encountered_value;
        }
        if (encountered_value < lower_bound) {
            lower_bound = encountered_value;
        }
    }
    return RangeT(lower_bound, upper_bound);
}

template <>
StringValueRange FindRangeOf<model::String, model::StringValueRange>(
        model::TypedColumnData const& column) {
    auto domain = std::vector<String>();
    for (size_t row_index = 0; row_index < column.GetNumRows(); ++row_index) {
        std::byte const* value = column.GetValue(row_index);
        std::string string_value = model::Type::GetValue<std::string>(value);
        bool first_occurrence =
                std::find(domain.begin(), domain.end(), string_value) == domain.end();
        if (first_occurrence) {
            domain.push_back(std::move(string_value));
        }
    }
    return StringValueRange(domain);
}

StringValueRange::StringValueRange(TypedColumnData const& column) {
    *this = std::move(FindRangeOf<String, StringValueRange>(column));
}

DoubleValueRange::DoubleValueRange(TypedColumnData const& column) {
    *this = std::move(FindRangeOf<Double, DoubleValueRange>(column));
}
IntValueRange::IntValueRange(TypedColumnData const& column) {

    *this = std::move(FindRangeOf<Int, IntValueRange>(column));
}

std::shared_ptr<ValueRange> CreateValueRange(model::TypedColumnData const& column) {
    switch (column.GetTypeId()) {
        case TypeId::kString:
            return std::make_shared<StringValueRange>(column);
        case TypeId::kDouble:
            return std::make_shared<DoubleValueRange>(column);
        case TypeId::kInt:
            return std::make_shared<IntValueRange>(column);
        default:
            throw std::invalid_argument(std::string("Column has invalid type_id in function: ") +
                                        __func__);
    }
}

std::string StringValueRange::ToString() const {
    std::ostringstream result;
    result << "[";
    if (domain.size() > 0) {
        result << domain[0];
    }
    for (size_t i = 1; i < domain.size(); ++i) {
        result << ", " << domain[i];
    }
    result << "]";
    return result.str();
}

std::string DoubleValueRange::ToString() const {
    std::ostringstream result;
    result << "[" << lower_bound << " - " << upper_bound << "]";
    return result.str();
}

std::string IntValueRange::ToString() const {
    std::ostringstream result;
    result << "[" << lower_bound << " - " << upper_bound << "]";
    return result.str();
}

}  // namespace model
