#pragma once

#include "core/algorithms/od/order/sorted_partitions.h"
#include "core/util/better_enum_with_visibility.h"

namespace algos::order {

BETTER_ENUM(ValidityType, char, valid = 0, merge, swap);

ValidityType CheckForSwap(SortedPartition const& l, SortedPartition const& r);

}  // namespace algos::order
