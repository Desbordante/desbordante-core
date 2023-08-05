#include "column_combination.h"

#include <sstream>

namespace model {

std::string ColumnCombination::ToString() const {
    std::vector<ColumnIndex> const& col_ids = GetColumnIndices();
    std::stringstream ss;
    for (auto it = col_ids.begin(); it != col_ids.end(); ++it) {
        if (it != col_ids.begin()) {
            ss << ", ";
        }
        ss << GetTableIndex() << '.' << *it;
    }
    return ss.str();
}

}  // namespace model
