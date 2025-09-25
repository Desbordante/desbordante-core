#pragma once
#include <list>
#include <string_view>

#include "algorithms/dd/dd.h"

namespace util::dd {
model::DFStringConstraint CreateDf(std::string_view const& col_name, double lower_bound,
                                   double upper_bound);

model::DDString CreateDd(std::list<model::DFStringConstraint> const& lhs,
                         std::list<model::DFStringConstraint> const& rhs);
}  // namespace util::dd
