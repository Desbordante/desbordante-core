#pragma once

#include <cstddef>
#include <limits>
#include <vector>

namespace algos::sd_verifier::ds {

// The segment tree stores minimum cost nodes over ranges of compressed
// coordinate values.
struct RmqNode {
    double val;       // Key for comparison: T(j) - j - a_j/G2 (or T(j) - j for G2=inf)
    size_t rank;      // Rank in compressed coordinates
    long source_idx;  // Original index j in the array
    long t_val;       // Actual T(j) value for exact cost computation
    long j_val;       // Actual j value for exact cost computation

    bool operator<(RmqNode const& other) const {
        return val < other.val;
    }
};

// Cost node for tracking minimum cost and source index.
struct CostNode {
    long cost;        // Key for comparison: T(j) - j - round(a_j/G) for exact gap
    long source_idx;  // Original index j in the array
    long t_val;       // Actual T(j) value for exact cost computation
    long j_val;       // Actual j value for exact cost computation
};

// The tree is built over coordinate-compressed values, allowing
// lookups of the minimum cost among predecessors with values in a given range.
class SegmentTree {
private:
    size_t n_;
    std::vector<RmqNode> tree_;

public:
    explicit SegmentTree(size_t n)
        : n_(n),
          tree_(2 * n, {std::numeric_limits<double>::infinity(), std::numeric_limits<size_t>::max(),
                        -1, 0, 0}) {}

    void Update(size_t pos, double value, size_t rank, long source_idx, long t_val, long j_val);
    RmqNode Query(size_t l, size_t r) const;
};

// Used for the exact gap case (G1 == G2) where elements are grouped into
// equivalence classes based on their modulo G1 remainder.
class Fenwick {
private:
    size_t n_;
    std::vector<CostNode> tree_;

public:
    explicit Fenwick(size_t n)
        : n_(n), tree_(n + 1, {std::numeric_limits<long>::max(), -1, 0, 0}) {}

    void Update(size_t pos, long value, long source_idx, long t_val, long j_val);
    CostNode Query(size_t pos) const;
};

}  // namespace algos::sd_verifier::ds
