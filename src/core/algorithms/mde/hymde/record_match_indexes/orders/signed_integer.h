#pragma once

#include <cstdint>

#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::orders {
class SignedInteger : public TotalOrder<std::intmax_t> {};
}  // namespace algos::hymde::record_match_indexes::orders
