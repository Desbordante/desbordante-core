#pragma once

#include <cstddef>

#include "core/model/index.h"

namespace algos::hymd::utility {
// swap should swap indices too
void InversePermutation(std::size_t size, auto&& index_at, auto&& swap) {
    if (size < 2) return;
    // Last element is either already placed correctly and not processed or is processed during
    // previous iterations and correctly placed last, so we don't need to start the inner loop for
    // it.
    --size;
    model::Index i = 0;
    do {
        for (model::Index old_index = index_at(i); old_index != i; old_index = index_at(i)) {
            swap(i, old_index);
        }
    } while (++i != size);
}
}  // namespace algos::hymd::utility
