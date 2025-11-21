#pragma once

#include <magic_enum/magic_enum.hpp>

#include "sorted_partitions.h"

namespace algos::order {

enum class ValidityType : char { valid = 0, merge, swap };

ValidityType CheckForSwap(SortedPartition const& l, SortedPartition const& r);

}  // namespace algos::order
