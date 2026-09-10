#include <algorithm>
#include <unordered_map>

#include <boost/container_hash/hash.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

namespace gspan {

// The Sparse Triangular Matrix is a hashmap where the key is a pair of items {I, J}
// (normalized such that the order of I and J does not matter)
// and the value is the count of the pair {I, J}.
class SparseTriangularMatrix {
    using Key = std::pair<int, int>;
    boost::unordered_flat_map<Key, int, boost::hash<Key>> matrix_;

    static Key Normalize(int i, int j) noexcept {
        return std::minmax(i, j);
    }

public:
    SparseTriangularMatrix() = default;

    void IncrementCount(int i, int j) {
        ++matrix_[Normalize(i, j)];
    }

    int GetSupport(int i, int j) const noexcept {
        auto it = matrix_.find(Normalize(i, j));
        return it == matrix_.end() ? 0 : it->second;
    }

    void SetSupport(int i, int j, int sup) {
        matrix_[Normalize(i, j)] = sup;
    }

    void RemoveInfrequent(int minsup) {
        boost::unordered::erase_if(matrix_, [&](auto const& kv) { return kv.second < minsup; });
    }
};

}  // namespace gspan