#pragma once

#include <magic_enum/magic_enum.hpp>

#include "core/algorithms/od/order/sorted_partitions.h"

namespace algos::order {

enum class ValidityType : char { kValid = 0, kMerge, kSwap };

ValidityType CheckForSwap(SortedPartition const& l, SortedPartition const& r);

}  // namespace algos::order
