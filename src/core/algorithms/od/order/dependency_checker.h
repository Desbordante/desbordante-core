#pragma once

#include "core/algorithms/od/order/sorted_partitions.h"

namespace algos::order {

enum class ValidityType : char { kValid = 0, kMerge, kSwap };

ValidityType CheckForSwap(SortedPartition const& l, SortedPartition const& r);

}  // namespace algos::order
