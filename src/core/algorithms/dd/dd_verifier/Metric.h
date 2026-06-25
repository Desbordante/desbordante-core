#pragma once

#include <cstddef>

#include "core/util/export.h"

class DESBORDANTE_EXPORT Metric {
public:
    virtual double Dist(std::byte const* first, std::byte const* second) const = 0;
    virtual ~Metric() = default;
};
