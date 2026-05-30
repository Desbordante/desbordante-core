#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "core/model/table/column.h"
#include "core/model/table/vertical.h"

namespace pac::util {
inline std::string ColumnNamesToString(std::vector<std::string> const& column_names) {
    std::ostringstream oss;
    oss << '[';
    for (auto it = column_names.begin(); it != column_names.end(); ++it) {
        if (it != column_names.begin()) {
            oss << ' ';
        }
        oss << *it;
    }
    oss << ']';
    return oss.str();
}

inline std::vector<std::string> GetColumnNames(Vertical const& vertical) {
    std::vector<std::string> result;
    result.reserve(vertical.GetArity());
    std::ranges::transform(vertical.GetColumns(), std::back_inserter(result),
                           [](Column const* col) { return col->GetName(); });
    return result;
}
}  // namespace pac::util
