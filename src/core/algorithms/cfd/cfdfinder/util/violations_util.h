#pragma once

#include <vector>

#include "cfd/cfdfinder/types/enriched_plis.h"
#include "fd/hycommon/types.h"

namespace algos::cfdfinder::util {
size_t CalculateViolations(Cluster const& cluster, hy::Row const& inverted_rhs_pli);
}  // namespace algos::cfdfinder::util