#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace gspan {

// the triangular matrix is a hashmap of hashmaps
// where the key is an item I, then the value is a map where each entry is a key representing an
// item J and a value representing the count of {I, J}.
class SparseTriangularMatrix {
    std::unordered_map<int, std::unordered_map<int, int>> matrix_;

public:
    SparseTriangularMatrix() = default;

    void IncrementCount(int i, int j);
    int GetSupport(int i, int j) const;
    void SetSupport(int i, int j, int sup);
    void RemoveInfrequent(int minsup);
};

}  // namespace gspan