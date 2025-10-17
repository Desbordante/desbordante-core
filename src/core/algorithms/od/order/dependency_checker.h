#pragma once

#include <enum.h>

#include "od/order/dependency_checker.h"
#include "sorted_partitions.h"

namespace algos {
namespace order {
class SortedPartition;
}  // namespace order
}  // namespace algos

namespace algos::order {

BETTER_ENUM(ValidityType, char, valid = 0, merge, swap);

ValidityType CheckForSwap(SortedPartition const& l, SortedPartition const& r);

}  // namespace algos::order
