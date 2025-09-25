#include "column_combination.h"

#include <algorithm>
#include <iosfwd>
#include <ostream>
#include <sstream>

#include "table/column_index.h"

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

bool ColumnCombination::HaveIndicesIntersection(ColumnCombination const& lhs,
                                                ColumnCombination const& rhs) noexcept {
    if (lhs.GetTableIndex() != rhs.GetTableIndex()) return false;

    std::vector<ColumnIndex> const& lhs_ids = lhs.GetColumnIndices();
    std::vector<ColumnIndex> const& rhs_ids = rhs.GetColumnIndices();
    return std::any_of(lhs_ids.begin(), lhs_ids.end(), [&rhs_ids](ColumnIndex i) {
        return std::find(rhs_ids.begin(), rhs_ids.end(), i) != rhs_ids.end();
    });
}

}  // namespace model
