#pragma once

#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/model/table/position_list_index.h"

namespace model {
using AttributeMask = boost::dynamic_bitset<>;
using AttributeMaskSet = std::unordered_set<AttributeMask>;

AttributeMaskSet ComputeAgreeSets(std::vector<PositionListIndex> const& plis);
}  // namespace model
