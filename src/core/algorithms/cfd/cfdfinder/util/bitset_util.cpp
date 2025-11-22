#include "algorithms/cfd/cfdfinder/util/bitset_util.h"

namespace algos::cfdfinder::util {
void ForEachSetBit(BitSet const& bitset, std::function<void(std::size_t)>&& fun) {
    for (size_t attr = bitset.find_first(); attr != BitSet::npos; attr = bitset.find_next(attr)) {
        fun(attr);
    }
}
}  // namespace algos::cfdfinder::util
