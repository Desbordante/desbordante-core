#pragma once
#include "algorithms/dd/dd.h"

namespace util {
    namespace dd {
        model::DFStringConstraint createDF(const std::string &col_name, double lower_bound, double upper_bound);

        model::DDString createDD(const std::list<model::DFStringConstraint>& lhs,
                                  const std::list<model::DFStringConstraint>& rhs);
    }
}
