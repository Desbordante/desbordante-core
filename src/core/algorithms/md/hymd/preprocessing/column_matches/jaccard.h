#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>

#include "algorithms/md/hymd/preprocessing/column_matches/pairwise.h"
#include "algorithms/md/hymd/utility/intersection_size.h"

namespace algos::hymd::preprocessing::column_matches {
namespace similarity_measures {
template <typename T>
double JaccardIndex(std::unordered_set<T> const& set1, std::unordered_set<T> const& set2) {
    std::size_t const first_size = set1.size();
    std::size_t const second_size = set2.size();
    bool const first_empty = first_size == 0;
    bool const second_empty = second_size == 0;
    if (first_empty && second_empty) return 1.0;
    if (first_empty || second_empty) return 0.0;

    std::size_t const intersection_size = first_size > second_size
                                                  ? utility::IntersectionSize(set2, set1)
                                                  : utility::IntersectionSize(set1, set2);
    std::size_t const union_size = first_size + second_size - intersection_size;

    return static_cast<double>(intersection_size) / union_size;
}

double StringJaccardIndex(std::string const& s1, std::string const& s2);
}  // namespace similarity_measures

class Jaccard : public NormalPairwise<similarity_measures::StringJaccardIndex> {
    static constexpr auto kName = "jaccard";

public:
    template <typename... Args>
    Jaccard(Args&&... args)
        : NormalPairwise<similarity_measures::StringJaccardIndex>(kName,
                                                                  std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::column_matches
