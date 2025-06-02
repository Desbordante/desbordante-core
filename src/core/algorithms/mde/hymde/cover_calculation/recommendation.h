#pragma once

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "util/py_tuple_hash.h"

namespace algos::hymde::cover_calculation {
struct Recommendation {
    record_match_indexes::PartitionIndex::Clusters const* left_record_clusters;
    record_match_indexes::PartitionIndex::Clusters const* right_record_clusters;

    friend bool operator==(Recommendation const& a, Recommendation const& b) {
        return *a.left_record_clusters == *b.left_record_clusters &&
               *a.right_record_clusters == *b.right_record_clusters;
    }
};

using Recommendations = std::vector<Recommendation>;
}  // namespace algos::hymde::cover_calculation

namespace std {
// This is probably unneeded, as it is unlikely to have two pairs where all
// values are the same in one dataset, pointer comparison should suffice. If
// there do happen to be pairs like this, they wouldn't cause as much of a
// slowdown as hashing all this does.
template <>
struct hash<algos::hymde::cover_calculation::Recommendation> {
    std::size_t operator()(
            algos::hymde::cover_calculation::Recommendation const& p) const noexcept {
        using Clusters = algos::hymde::record_match_indexes::PartitionIndex::Clusters;
        using algos::hymde::PartitionValueId;
        Clusters const& left_record = *p.left_record_clusters;
        Clusters const& right_record = *p.right_record_clusters;
        util::PyTupleHash hasher{left_record.size() + right_record.size()};
        for (PartitionValueId v : left_record) {
            hasher.AddValue(v);
        }
        for (PartitionValueId v : right_record) {
            hasher.AddValue(v);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
