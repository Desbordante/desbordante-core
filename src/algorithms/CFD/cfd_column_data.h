#include <utility>
#include <vector>
#include "../../model/abstract_column_data.h"
using NumToken = int;

class CFDColumnData : model::AbstractColumnData
{
    std::vector<NumToken> values;
    public:
        CFDColumnData(Column const* col, std::vector<int>  col_values) : AbstractColumnData(col), values(std::move(col_values)) {
    }
    explicit CFDColumnData(Column const* col) : AbstractColumnData(col) {
    }
    std::vector<NumToken> const& GetValues() const { return values; }

    std::string ToString() const final { return "Data for " + column_->ToString(); }
};