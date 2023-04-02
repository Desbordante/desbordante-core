#include <algorithm>

// see ../algorithms/cfd/LICENSE

#include "algorithms/cfd/util/set_util.h"

std::vector<int> Range(int min, int max, int step) {
    std::vector<int> res;
    res.reserve((max - min) / step);
    for (int i = min; i != max; i += step) {
        res.push_back(i);
    }
    return res;
}