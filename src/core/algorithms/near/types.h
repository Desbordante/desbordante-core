#pragma once

#include <cstddef>

using FeatureIndex = unsigned int;
struct Consequence {
    FeatureIndex feature;
    bool positive;
};
// O stands for Oredered (by frequency).
using OFeatureIndex = unsigned int;
struct OConsequence {
    OFeatureIndex feature;
    bool positive;
};
