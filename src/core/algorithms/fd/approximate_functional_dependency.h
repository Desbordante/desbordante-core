#pragma once

#include <vector>

#include "core/algorithms/fd/afd_measure.h"
#include "core/model/table/attribute.h"

namespace model {
// measure(table, (lhs, rhs)) >= measure_score
struct ApproximateFunctionalDependency {
    std::string table_name;
    std::vector<Attribute> lhs;
    std::vector<Attribute> rhs;
    AfdMeasure measure;
    double measure_score;  // error is 1 - measure_score
};
}  // namespace model
