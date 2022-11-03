#pragma once

#include <boost/dynamic_bitset.hpp>
#include <unordered_set>
#include <vector>

#include "non_fd_list.h"

namespace algos::hyfd {

/**
 * Collection of fd-violating column combinations found by the Sampler.
 *
 * Stores combinations found during the lifetime as well as combinations found since
 * the last access.
 */
class NonFds {};

}  // namespace algos::hyfd