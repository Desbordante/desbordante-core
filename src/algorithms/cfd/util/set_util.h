#pragma once

// see ../algorithms/cfd/LICENSE

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

std::vector<int> Range(int, int, int = 1);

template <typename T>
void Shuffle(T& collection) {
    std::mt19937 gen(time(0));
    std::shuffle(collection.begin(), collection.end(), gen);
}

template <typename T>
void Shuffle(T& collection, unsigned seed) {
    std::mt19937 gen(seed);
    std::shuffle(collection.begin(), collection.end(), gen);
}
