#pragma once

#include "core/algorithms/cfd/cfdfinder/types/cluster.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder::utils {
size_t CalculateViolations(Cluster const& cluster, Row const& inverted_rhs_pli);
}  // namespace algos::cfdfinder::utils
