#pragma once

#include <cstddef>

#include "algorithms/cfd/cfdfinder/types/cluster.h"
#include "algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder::util {
size_t CalculateViolations(Cluster const& cluster, Row const& inverted_rhs_pli);
}  // namespace algos::cfdfinder::util
