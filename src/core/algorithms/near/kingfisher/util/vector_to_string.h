#pragma once

#include <string>
#include <vector>
#include <sstream>

template<typename T>
std::string VectorToString(std::vector<T> const& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];  // Convert FeatureIndex to string
        if (i < vec.size() - 1) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}