#pragma once

#include <enum.h>

namespace algos {

// clang-format off
/* Enumeration of all supported non-pipeline algorithms. If you implement a new
 * algorithm please add its corresponding value to this enum and to the type
 * tuple above.
 * NOTE: algorithm string name representation is taken from the value in this
 * enum, so name it appropriately (lowercase and without additional symbols).
 */
BETTER_ENUM(AlgorithmType, char,
/* Functional dependency mining algorithms */
    depminer = 0,
    dfd,
    fastfds,
    fdep,
    fdmine,
    pyro,
    tane,
    fun,
    hyfd,
    aidfd,

/* Association rules mining algorithms */
    apriori,

/* Metric verifier algorithm */
    metric,

/* Statistic algorithms */
    stats,

/* FD verifier algorithm */
    fd_verifier,

/* Unique Column Combination mining algorithms */
    hyucc
)

// clang-format on

}  // namespace algos
