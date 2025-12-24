#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::ar_verifier {

enum class ClusterPriority : char {
    kFullLeftFullRight = -1,
    kFullLeftPartialRight,
    kFullLeftNoRight,
    kPartialLeftFullRight,
    kPartialLeftPartialRight,
    kPartialLeftNoRight,
};

}  // namespace algos::ar_verifier
