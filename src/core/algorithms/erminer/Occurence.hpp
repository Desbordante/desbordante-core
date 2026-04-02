#pragma once
#include <cstdint>

struct Occurence {
    int16_t firstItemset;
    int16_t lastItemset;
    
    Occurence();
    Occurence(int16_t firstItemset, int16_t lastItemset);
};