#include "algorithms/nd/nd.h"

#include <sstream>

std::string ND::ToShortString() const {
    auto indices_to_short_string =
            [](std::vector<model::ColumnIndex> const& indices) -> std::string {
        std::stringstream ss;
        ss << '[';
        for (auto ptr{indices.begin()}; ptr != indices.end(); ++ptr) {
            if (ptr != indices.begin()) {
                ss << ", ";
            }
            ss << *ptr;
        }
        ss << ']';
        return ss.str();
    };

    std::stringstream ss;
    ss << indices_to_short_string(GetLhsIndices()) << " -> "
       << indices_to_short_string(GetRhsIndices());
    return ss.str();
}

std::string ND::ToLongString() const {
    auto vert_to_long_string = [](Vertical vert) -> std::string {
        std::stringstream ss;
        ss << '[';
        auto const& columns = vert.GetColumns();
        for (auto ptr{columns.begin()}; ptr != columns.end(); ++ptr) {
            if (ptr != columns.begin()) {
                ss << ", ";
            }
            ss << (*ptr)->GetName();
        }
        ss << ']';
        return ss.str();
    };

    std::stringstream ss;
    ss << vert_to_long_string(GetLhs()) << " -> " << vert_to_long_string(GetRhs());
    return ss.str();
}
