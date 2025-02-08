#pragma once

namespace algos::ar_verifier::util {
enum class ClusterPriority {
    kFullLeftFullRight = 0,  // will not be in the cluster
    kFullLeftPartialRight = 1,
    kFullLeftNoRight = 2,
    kPartialLeftFullRight = 3,
    kPartialLeftPartialRight = 4,
    kPartialLeftNoRight = 5
};
}