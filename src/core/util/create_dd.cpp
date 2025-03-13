#include "create_dd.h"

namespace util {
model::DFStringConstraint dd::CreateDf(std::string const& col_name, double const lower_bound,
                                       double const upper_bound) {
    model::DFStringConstraint dd;
    dd.column_name = col_name;
    dd.constraint.lower_bound = lower_bound;
    dd.constraint.upper_bound = upper_bound;
    return dd;
}

model::DDString dd::CreateDd(std::list<model::DFStringConstraint> const& lhs,
                             std::list<model::DFStringConstraint> const& rhs) {
    model::DDString dd;
    dd.left = lhs;
    dd.right = rhs;
    return dd;
}
}  // namespace util
