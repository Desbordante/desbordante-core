#include "algorithms/nd/util/get_vertical_names.h"

#include <algorithm>
#include <string>
#include <vector>

#include "model/table/column.h"
#include "model/table/vertical.h"

namespace algos::nd::util {

[[nodiscard]] std::vector<std::string> GetVerticalNames(Vertical const& vert) {
    std::vector<std::string> result;
    std::vector<Column const*> const& cols = vert.GetColumns();
    std::transform(cols.begin(), cols.end(), std::back_inserter(result),
                   [](Column const* col) { return col->GetName(); });
    return result;
}

}  // namespace algos::nd::util
