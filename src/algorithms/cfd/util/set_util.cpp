#include <algorithm>
#include <numeric>

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

// Возвращает список вида 0 1 2 ... размера max
std::vector<int> Iota(int max) {
    std::vector<int> iotas(max);
    std::iota(iotas.begin(), iotas.end(), 0);
    return iotas;
}

void SubsetsLengthK(const int size, const int k, std::vector<std::bitset<32>>& subs) {
    const int max_b = pow(2, size);
    int x = pow(2, k) - 1;
    while (x < max_b) {
        subs.push_back(std::bitset<32>(x));
        int u = x & (-x);
        int v = x + u;
        x = v + (((v ^ x) / u) >> 2);
    }
}
