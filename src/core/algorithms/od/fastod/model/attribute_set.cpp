#include "core/algorithms/od/fastod/model/attribute_set.h"

#include <sstream>

namespace algos::fastod {

std::string AttributeSet::ToString() const {
    std::stringstream result;
    result << "{";

    bool first = true;

    Iterate([&result, &first](model::ColumnIndex i) {
        if (first)
            first = false;
        else
            result << ",";

        result << i + 1;
    });

    result << "}";

    return result.str();
}

void AttributeSet::Iterate(std::function<void(model::ColumnIndex)> callback) const {
    for (model::ColumnIndex attr = FindFirst(); attr != Size(); attr = FindNext(attr)) {
        callback(attr);
    }
}

std::vector<model::ColumnIndex> AttributeSet::AsVector() const {
    std::vector<model::ColumnIndex> attrs;
    attrs.reserve(Count());
    Iterate([&attrs](model::ColumnIndex attr) { attrs.push_back(attr); });
    return attrs;
}

}  // namespace algos::fastod
