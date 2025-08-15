#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::orders {
// There is no point in making a separate named class here. There are only two possible orders on
// bools, so there is no significant invariant to name.
using Bool = TotalOrder<bool>;
}  // namespace algos::hymde::record_match_indexes::orders

