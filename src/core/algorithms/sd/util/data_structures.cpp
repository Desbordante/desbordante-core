#include "data_structures.h"

#include <algorithm>

namespace algos::sd::util {

void SegmentTree::Update(size_t pos, long double value, size_t rank, long source_idx, long t_val,
                         long j_val) {
    pos += n_;
    tree_[pos] = {value, rank, source_idx, t_val, j_val};
    for (pos >>= 1; pos > 0; pos >>= 1) {
        tree_[pos] = std::min(tree_[pos << 1], tree_[(pos << 1) | 1]);
    }
}

std::optional<RmqNode> SegmentTree::Query(size_t l, size_t r) const {
    std::optional<RmqNode> res;
    for (l += n_, r += n_; l < r; l >>= 1, r >>= 1) {
        if (l & 1) {
            if (!res.has_value() || tree_[l] < res.value()) res = tree_[l];
            l++;
        }
        if (r & 1) {
            --r;
            if (!res.has_value() || tree_[r] < res.value()) res = tree_[r];
        }
    }
    if (!res.has_value() || res->source_idx == -1) {
        return std::nullopt;
    }
    return res;
}

void Fenwick::Update(size_t pos, long double value, long source_idx, long t_val, long j_val) {
    CostNode const candidate{value, source_idx, t_val, j_val};
    for (++pos; pos <= n_; pos += pos & -pos) {
        if (candidate < tree_[pos]) {
            tree_[pos] = candidate;
        }
    }
}

std::optional<CostNode> Fenwick::Query(size_t pos) const {
    CostNode res = {std::numeric_limits<long double>::infinity(), -1, 0, 0};
    for (++pos; pos > 0; pos -= pos & -pos) {
        if (tree_[pos] < res) {
            res = tree_[pos];
        }
    }
    if (res.source_idx == -1) {
        return std::nullopt;
    }
    return res;
}

}  // namespace algos::sd::util
