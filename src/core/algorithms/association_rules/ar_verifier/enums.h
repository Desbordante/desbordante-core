#pragma once

#include <enum.h>

namespace algos::ar_verifier {
BETTER_ENUM(ClusterPriority, char, full_left_full_right = -1, full_left_partial_right,
            full_left_no_right, partial_left_full_right, partial_left_partial_right,
            partial_left_no_right)
}
