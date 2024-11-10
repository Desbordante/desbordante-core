#pragma once

#include <utility>
#include <variant>
#include <vector>

namespace algos::hymd::preprocessing::column_matches {
template <typename L, typename R>
struct TransformedColumnsHolder {
    using LeftVec = std::vector<L>;
    using RightVec = std::vector<R>;
    using VecPair = std::pair<LeftVec, RightVec>;

    std::variant<LeftVec, std::pair<LeftVec, RightVec>> values;
    LeftVec const* left_ptr;
    RightVec const* right_ptr;

    TransformedColumnsHolder(LeftVec left_vec)
        : values(std::move(left_vec)),
          left_ptr(std::get_if<LeftVec>(&values)),
          right_ptr(left_ptr) {}

    TransformedColumnsHolder(LeftVec left_vec, RightVec right_vec)
        : values(std::in_place_type<VecPair>, std::move(left_vec), std::move(right_vec)),
          left_ptr(&std::get<VecPair>(values).first),
          right_ptr(&std::get<VecPair>(values).second) {}

    TransformedColumnsHolder(LeftVec const* left_ptr, RightVec const* right_ptr)
        : left_ptr(left_ptr), right_ptr(right_ptr) {}
};

}  // namespace algos::hymd::preprocessing::column_matches
