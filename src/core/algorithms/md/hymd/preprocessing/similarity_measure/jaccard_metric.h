#pragma once

#include <algorithm>
#include <set>
#include <string>

namespace algos::hymd::preprocessing::similarity_measure {
template <typename T>
double JaccardIndex(std::set<T> const& set1, std::set<T> const& set2) {
    if (set1.empty() && set2.empty()) return 1.0;

    std::set<T> intersection;
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                          std::inserter(intersection, intersection.begin()));
    size_t union_size = set1.size() + set2.size() - intersection.size();

    return union_size == 0 ? 1.0 : static_cast<double>(intersection.size()) / union_size;
}

double JaccardIndex(std::string const& s1, std::string const& s2);
}  // namespace algos::hymd::preprocessing::similarity_measure
