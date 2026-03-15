#pragma once
#include <vector>

namespace algos::fd::fdhits {

template <typename E>
class VecEdgeCollector {
public:
    std::vector<E> results;

    void Consume(E const& edge) {
        results.push_back(edge);
    }

    size_t Size() const {
        return results.size();
    }

    auto begin() const {
        return results.begin();
    }

    auto end() const {
        return results.end();
    }
};

}  // namespace algos::fd::fdhits
