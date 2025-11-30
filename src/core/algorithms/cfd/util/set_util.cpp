#include "core/algorithms/cfd/util/set_util.h"

#include <algorithm>
#include <numeric>

// see algorithms/cfd/LICENSE

namespace algos::cfd {

// Returns a list that looks like this - min, min + step, min + 2 * step, ... max(not included)
// Size of this list is operations_count
std::vector<int> Range(int min, int max, int step) {
    int operations_count = (max - min) % step == 0 ? (max - min) / step : ((max - min) / step) + 1;
    std::vector<int> res(operations_count);
    min -= step;
    std::generate(res.begin(), res.end(), [&min, step] { return min += step; });
    return res;
}

// Returns list that looks like this - 0 1 2 ... Size of this list is max variable.
std::vector<int> Iota(unsigned max) {
    std::vector<int> iotas(max);
    std::iota(iotas.begin(), iotas.end(), 0);
    return iotas;
}
}  // namespace algos::cfd
