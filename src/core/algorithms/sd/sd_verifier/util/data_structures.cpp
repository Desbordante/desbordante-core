#include "data_structures.h"

namespace algos::sd_verifier::ds {

void SegmentTree::Update(size_t pos, double value, size_t rank, long source_idx, long t_val,
                         long j_val) {
    pos += n_;
    tree_[pos] = {value, rank, source_idx, t_val, j_val};
    for (pos >>= 1; pos > 0; pos >>= 1) {
        tree_[pos] = std::min(tree_[pos << 1], tree_[(pos << 1) | 1]);
    }
}

RmqNode SegmentTree::Query(size_t l, size_t r) const {
    RmqNode res = {std::numeric_limits<double>::infinity(), std::numeric_limits<size_t>::max(), -1,
                   0, 0};
    for (l += n_, r += n_; l < r; l >>= 1, r >>= 1) {
        if (l & 1) res = std::min(res, tree_[l++]);
        if (r & 1) res = std::min(res, tree_[--r]);
    }
    return res;
}

void Fenwick::Update(size_t pos, long value, long source_idx, long t_val, long j_val) {
    for (++pos; pos <= n_; pos += pos & -pos) {
        if (value < tree_[pos].cost) {
            tree_[pos] = {value, source_idx, t_val, j_val};
        }
    }
}

CostNode Fenwick::Query(size_t pos) const {
    CostNode res = {std::numeric_limits<long>::max() / 2, -1, 0, 0};
    for (++pos; pos > 0; pos -= pos & -pos) {
        if (tree_[pos].cost < res.cost) {
            res = tree_[pos];
        }
    }
    return res;
}

}  // namespace algos::sd_verifier::ds
