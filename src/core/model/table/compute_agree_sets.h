#pragma once

#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/model/table/stripped_partitions.h"

namespace model {
std::unordered_set<boost::dynamic_bitset<>> ComputeAgreeSets(StrippedPartitions const& plis);
}  // namespace model
