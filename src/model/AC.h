#pragma once

#include <boost/function.hpp>

namespace model {

struct AC {
    AC(std::pair<int, int> l, std::pair<int, int> r, const std::byte* la, const std::byte* ra,
       std::byte* res)
        : lhs_index(l), rhs_index(r), lhs_arg(la), rhs_arg(ra), res(res) {}

    AC(const AC&) = default;

    ~AC() {
        delete[] res;
    }

    std::pair<std::size_t, std::size_t> lhs_index;
    std::pair<std::size_t, std::size_t> rhs_index;
    const std::byte* lhs_arg;
    const std::byte* rhs_arg;
    std::byte* res;
};

}  // namespace model