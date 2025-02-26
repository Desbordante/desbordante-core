#include "create_dd.h"

namespace util {
    model::DFStringConstraint dd::createDF(const std::string &col_name, const double lower_bound, const double upper_bound) {
        model::DFStringConstraint dd;
        dd.column_name = col_name;
        dd.lower_bound = lower_bound;
        dd.upper_bound = upper_bound;
        return dd;
    }

    model::DDString dd::createDD(const std::list<model::DFStringConstraint>& lhs,
                                  const std::list<model::DFStringConstraint>& rhs) {
        model::DDString dd;
        dd.left = lhs;
        dd.right = rhs;
        return dd;

    }
}
