#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<std::size_t>> {
    std::size_t operator()(std::vector<std::size_t> const& p) const noexcept {
        util::PyTupleHash hasher{p.size()};
        for (std::size_t el : p) {
            hasher.AddValue(el);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
