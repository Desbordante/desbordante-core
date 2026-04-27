#pragma once
#include "Prefix.hpp"
#include "Bitmap.hpp"
#include <vector>

class Candidate {
public:
    Prefix prefix;
    Bitmap bitmap;
    std::vector<int> sn;
    std::vector<int> in;
    int hasToBeGreaterThanForIStep;
    int candidateLength;
    int cachedSupport;

    Candidate(Prefix prefix, Bitmap bitmap, std::vector<int> sn,
             std::vector<int> in, int hasToBeGreaterThanForIStep, 
             int candidateLength, int support);

    bool operator<(const Candidate& other) const;
};