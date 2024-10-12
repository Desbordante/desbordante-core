#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "model/index.h"
#include "util/py_tuple_hash.h"

namespace algos::hymd {
struct LhsNode {
    model::Index child_array_index;
    ColumnClassifierValueId ccv_id;

    friend bool operator==(LhsNode const& l, LhsNode const& r) {
        return l.child_array_index == r.child_array_index && l.ccv_id == r.ccv_id;
    }
};

class MdLhs {
    using Nodes = std::vector<LhsNode>;
    Nodes values_;

public:
    using FasterType = void;
    using iterator = Nodes::const_iterator;

    // Placeholder for empty LHS.
    MdLhs() = default;

    // Cardinality must not exceed this value.
    MdLhs(std::size_t max_values) {
        values_.reserve(max_values);
    }

    ColumnClassifierValueId& AddNext(model::Index child_array_index) {
        return values_.emplace_back(child_array_index).ccv_id;
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

    friend bool operator==(MdLhs const& lhs1, MdLhs const& lhs2) {
        return lhs1.values_ == lhs2.values_;
    }

    std::size_t Cardinality() const noexcept {
        return values_.size();
    }

    bool IsEmpty() const noexcept {
        return values_.empty();
    }
};

}  // namespace algos::hymd

namespace std {
template <>
struct hash<algos::hymd::MdLhs> {
    std::size_t operator()(algos::hymd::MdLhs const& p) const noexcept {
        using model::Index, algos::hymd::ColumnClassifierValueId;
        util::PyTupleHash main_hasher(p.Cardinality());
        for (auto const& [child_array_index, ccv_id] : p) {
            util::PyTupleHash pair_hasher(2);
            pair_hasher.AppendHash(std::hash<Index>{}(child_array_index));
            pair_hasher.AppendHash(std::hash<ColumnClassifierValueId>{}(ccv_id));
            main_hasher.AppendHash(pair_hasher.GetResult());
        }
        return main_hasher.GetResult();
    }
};
}  // namespace std
