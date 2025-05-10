#pragma once

#include <vector>

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"
#include "util/py_tuple_hash.h"

namespace algos::hymde::cover_calculation::lattice {
struct LhsNode {
    model::Index offset;
    RecordClassifierValueId rcv_id = 0;

    friend bool operator==(LhsNode const& l, LhsNode const& r) {
        return l.offset == r.offset && l.rcv_id == r.rcv_id;
    }
};

class MdeLhs {
    using Nodes = std::vector<LhsNode>;
    Nodes values_;

public:
    using FasterType = void;
    using iterator = Nodes::const_iterator;

    // Placeholder for empty LHS.
    MdeLhs() = default;

    // Cardinality must not exceed this value.
    MdeLhs(std::size_t max_values) {
        values_.reserve(max_values);
    }

    RecordClassifierValueId& AddNext(model::Index offset) {
        values_.push_back({offset});
        return values_.back().rcv_id;
    }

    void RemoveLast() {
        values_.pop_back();
    }

    iterator begin() const noexcept {
        return values_.begin();
    }

    iterator end() const noexcept {
        return values_.end();
    }

    friend bool operator==(MdeLhs const& lhs1, MdeLhs const& lhs2) {
        return lhs1.values_ == lhs2.values_;
    }

    std::size_t Cardinality() const noexcept {
        return values_.size();
    }

    bool IsEmpty() const noexcept {
        return values_.empty();
    }
};

}  // namespace algos::hymde::cover_calculation::lattice

namespace std {
template <>
struct hash<algos::hymde::cover_calculation::lattice::MdeLhs> {
    std::size_t operator()(
            algos::hymde::cover_calculation::lattice::MdeLhs const& p) const noexcept {
        using model::Index, algos::hymde::RecordClassifierValueId;
        util::PyTupleHash main_hasher(p.Cardinality());
        for (auto const& [node_offset, rcv_id] : p) {
            util::PyTupleHash pair_hasher(2);
            pair_hasher.AppendHash(std::hash<Index>{}(node_offset));
            pair_hasher.AppendHash(std::hash<RecordClassifierValueId>{}(rcv_id));
            main_hasher.AppendHash(pair_hasher.GetResult());
        }
        return main_hasher.GetResult();
    }
};
}  // namespace std
