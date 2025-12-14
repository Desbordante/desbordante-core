#pragma once

#include <cstddef>

class Metric {
public:
    virtual double Dist(std::byte const* first, std::byte const* second) const = 0;
    virtual ~Metric() = default;
};
