#pragma once

#include <cstddef>

#include "core/model/index.h"

namespace util {
// This procedure is expected to be used when you have a collection of objects that was reordered
// and a permutation (e.g. array of their original positions). It will restore the original
// positions of the objects in-place. The permutation will be modified as well, becoming
// [0 1 2 ... n-1], where n is the number of elements.
// For example, suppose you had [obj_0, obj_1, obj_2, obj_3], then reordered that using the
// permutation [1 0 3 2]. You now have the array [obj_1, obj_0, obj_3, obj_2]. Giving the array and
// the permutation to this procedure will recover the original [obj_0, obj_1, obj_2, obj_3]
// ordering.
// [obj_1, obj_0, obj_3, obj_4], [1 0 3 2] -> [obj_0, obj_1, obj_2, obj_3]

// size indicates the size of the collection
// swap should swap both the index in the permutation and the objects themselves
// get_current_position should return the index of the element (which may have been modified by
// swap)
void UndoPermutation(std::size_t size, auto&& get_current_position, auto&& swap) {
    if (size < 2) return;
    // Last element is either already placed correctly and not processed or is processed during
    // previous iterations and correctly placed last, so we don't need to start the inner loop for
    // it.
    --size;
    model::Index target_index = 0;
    do {
        for (model::Index current_index = get_current_position(target_index);
             current_index != target_index; current_index = get_current_position(target_index)) {
            // swap should swap indices too
            swap(target_index, current_index);
        }
    } while (++target_index != size);
}
}  // namespace util
