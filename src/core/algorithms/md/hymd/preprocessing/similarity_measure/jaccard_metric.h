#pragma once

#include <algorithm>
#include <string>
#include <unordered_set>

#include "util/intersection_size.h"

namespace algos::hymd::preprocessing::similarity_measure {
template <typename T>
double JaccardIndex(std::unordered_set<T> const& set1, std::unordered_set<T> const& set2) {
    std::size_t const first_size = set1.size();
    std::size_t const second_size = set2.size();
    bool const first_empty = first_size == 0;
    bool const second_empty = second_size == 0;
    if (first_empty && second_empty) return 1.0;
    if (first_empty || second_empty) return 0.0;

    std::size_t const intersection_size = first_size > second_size
                                                  ? util::IntersectionSize(set2, set1)
                                                  : util::IntersectionSize(set1, set2);
    std::size_t const union_size = first_size + second_size - intersection_size;

    return static_cast<double>(intersection_size) / union_size;
}

double JaccardIndex(std::string const& s1, std::string const& s2);
}  // namespace algos::hymd::preprocessing::similarity_measure
