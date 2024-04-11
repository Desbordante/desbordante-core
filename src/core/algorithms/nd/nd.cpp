#include "algorithms/nd/nd.h"

#include <numeric>
#include <sstream>

namespace model {

std::string ND::ToShortString() const {
    auto indices_to_short_string =
            [](std::vector<model::ColumnIndex> const& indices) -> std::string {
        return '[' +
               std::accumulate(std::next(indices.begin()), indices.end(),
                               std::to_string(indices[0]),
                               [](std::string&& a, unsigned b) {
                                   return std::move(a) + ", " + std::to_string(b);
                               }) +
               ']';
    };

    std::stringstream stream;
    stream << indices_to_short_string(GetLhsIndices()) << " -> "
           << indices_to_short_string(GetRhsIndices());
    return stream.str();
}

std::string ND::ToLongString() const {
    auto cols_to_long_string = [](std::vector<Column const*> const& cols) -> std::string {
        return std::accumulate(std::next(cols.begin()), cols.end(), cols[0]->GetName(),
                               [](std::string&& a, Column const* b) {
                                   return std::move(a) + ", " + b->GetName();
                               });
    };

    std::stringstream stream;
    stream << cols_to_long_string(GetLhs().GetColumns()) << " -> "
           << cols_to_long_string(GetRhs().GetColumns());
    return stream.str();
}

}  // namespace model
