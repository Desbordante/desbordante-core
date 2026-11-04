#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

namespace algos::sd::util {

struct RmqNode {
    long double val;  // Scaled key: G2 * (T(j) - j) - a_j (or T(j) - j for G2=inf)
    size_t rank;      // Rank in compressed coordinates
    long source_idx;
    long t_val;
    long j_val;

    bool operator<(RmqNode const& other) const {
        if (val != other.val) {
            return val < other.val;
        }
        long const t_minus_j = t_val - j_val;
        long const other_t_minus_j = other.t_val - other.j_val;
        if (t_minus_j != other_t_minus_j) {
            return t_minus_j < other_t_minus_j;
        }
        return rank < other.rank;
    }
};

struct CostNode {
    long double cost;
    long source_idx;
    long t_val;
    long j_val;

    bool operator<(CostNode const& other) const {
        if (cost != other.cost) {
            return cost < other.cost;
        }
        long const t_minus_j = t_val - j_val;
        long const other_t_minus_j = other.t_val - other.j_val;
        if (t_minus_j != other_t_minus_j) {
            return t_minus_j < other_t_minus_j;
        }
        return source_idx < other.source_idx;
    }
};

class SegmentTree {
private:
    size_t n_;
    std::vector<RmqNode> tree_;

public:
    explicit SegmentTree(size_t n)
        : n_(n),
          tree_(2 * n, {std::numeric_limits<long double>::infinity(),
                        std::numeric_limits<size_t>::max(), -1, 0, 0}) {}

    void Update(size_t pos, long double value, size_t rank, long source_idx, long t_val,
                long j_val);
    std::optional<RmqNode> Query(size_t l, size_t r) const;
};

// Tracks minimum scaled costs inside groups with approximately equal modulo-G2 remainders.
class Fenwick {
private:
    size_t n_;
    std::vector<CostNode> tree_;

public:
    explicit Fenwick(size_t n)
        : n_(n), tree_(n + 1, {std::numeric_limits<long double>::infinity(), -1, 0, 0}) {}

    void Update(size_t pos, long double value, long source_idx, long t_val, long j_val);
    std::optional<CostNode> Query(size_t pos) const;
};

}  // namespace algos::sd::util
