#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

namespace algos::sd::util {

[[nodiscard]] inline long double ComparisonTolerance(long double lhs, long double rhs) {
    long double constexpr factor =
            static_cast<long double>(std::numeric_limits<double>::epsilon()) * 100.0L;
    return factor * std::max({1.0L, std::abs(lhs), std::abs(rhs)});
}

[[nodiscard]] inline double ParseNumeric(std::string const& raw) {
    size_t parsed = 0;
    double value = 0.0;
    try {
        value = std::stod(raw, &parsed);
    } catch (std::exception const&) {
        throw std::runtime_error("Failed to parse numeric value.");
    }
    for (size_t i = parsed; i < raw.size(); ++i) {
        if (!std::isspace(static_cast<unsigned char>(raw[i]))) {
            throw std::runtime_error("Failed to parse numeric value.");
        }
    }
    if (!std::isfinite(value)) {
        throw std::runtime_error("Numeric values must be finite.");
    }
    return value;
}

}  // namespace algos::sd::util
