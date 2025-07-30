#pragma once

#include <enum.h>  // for _map_index, _or_throw, cont...

#include "od/order/dependency_checker.h"  // for _name_array, _raw_names

namespace algos {
namespace order {
class SortedPartition;
}
}  // namespace algos

namespace algos::order {

BETTER_ENUM(ValidityType, char, valid = 0, merge, swap);

ValidityType CheckForSwap(SortedPartition const& l, SortedPartition const& r);

}  // namespace algos::order
