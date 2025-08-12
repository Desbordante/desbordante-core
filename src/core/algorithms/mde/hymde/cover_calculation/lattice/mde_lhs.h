#pragma once

#include <vector>

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"
#include "util/py_tuple_hash.h"

namespace algos::hymde::cover_calculation::lattice {
struct PathStep {
    model::Index offset;  // index of node in child node array
    RecordClassifierValueId rcv_id = 0;

    friend bool operator==(PathStep const& l, PathStep const& r) {
        return l.offset == r.offset && l.rcv_id == r.rcv_id;
    }
};

class PathToNode {
    using Steps = std::vector<PathStep>;
    Steps steps_;

public:
    using FasterType = void;
    using iterator = Steps::const_iterator;

    // Placeholder for empty LHS.
    PathToNode() = default;

    // Cardinality must not exceed this value.
    PathToNode(std::size_t max_values) {
        steps_.reserve(max_values);
    }

    RecordClassifierValueId& NextStep(model::Index offset) {
        steps_.push_back({offset});
        return steps_.back().rcv_id;
    }

    void RemoveLastStep() {
        steps_.pop_back();
    }

    iterator begin() const noexcept {
        return steps_.begin();
    }

    iterator end() const noexcept {
        return steps_.end();
    }

    friend bool operator==(PathToNode const& lhs1, PathToNode const& lhs2) {
        return lhs1.steps_ == lhs2.steps_;
    }

    std::size_t PathLength() const noexcept {
        return steps_.size();
    }
};

}  // namespace algos::hymde::cover_calculation::lattice

namespace std {
template <>
struct hash<algos::hymde::cover_calculation::lattice::PathToNode> {
    std::size_t operator()(
            algos::hymde::cover_calculation::lattice::PathToNode const& p) const noexcept {
        using model::Index, algos::hymde::RecordClassifierValueId;
        util::PyTupleHash main_hasher(p.PathLength());
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
