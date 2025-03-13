#pragma once
#include "algorithms/dd/dd.h"


    namespace util::dd {
        model::DFStringConstraint CreateDf(const std::string &col_name, double lower_bound, double upper_bound);

        model::DDString CreateDd(const std::list<model::DFStringConstraint>& lhs,
                                  const std::list<model::DFStringConstraint>& rhs);
    }

