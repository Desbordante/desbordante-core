#pragma once

#include <span>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_identifier.h"
#include "model/index.h"
#include "util/desbordante_assume.h"

namespace algos::hymde::record_match_indexes {
// Using the value from the comparison function and its corresponding total order, a total order can
// be defined on partitioning values of the right table and its records for each left table
// partitioning value.
// This structure stores cardinalities for a pair of related upper sets of these orders.
struct UpperSetCardinalities {
    // Given tables (r, s) a record classifier ((T, V, F, <=), l), and v_r \in Im_T(r)
    std::size_t record_set_cardinality;  // = |{ q \in s | l <= F(v_r, V(q)) }|
    std::size_t value_set_cardinality;   // = |{ v_s \in Im_V(s) | l <= F(v_r, v_s) }|
    // Im_T(x) is the image of set x for function T.
    // T and V are partitioning functions.
    // F is a comparison function, <= is a total order on its codomain.
    // l is a decision boundary.
    // v_r is a partitioning value for the table r (left table).
    // We call the first set the "record set" and the second the "value set". They are both upper
    // sets for a particularly defined total order.
    // Every value set corresponds to a record set that is the result of a union of (disjoint) sets
    // of records that have been assigned to the values' PLI clusters.
};

// For a left table partition value, a map from RCV ID to upper set cardinality info.
using LTPValueRCVIDUpperSetCardinalityMap =
        boost::container::flat_map<RecordClassifierValueId, UpperSetCardinalities>;
using RTPValueIDs = std::span<PartitionValueId const>;

struct MaterializedPValuesUpperSet {
    std::size_t record_set_cardinality;
    RTPValueIDs value_set_elements;

    bool IsEmpty() const noexcept {
        return record_set_cardinality == 0;
    }
};

// highest comparison value first
// TODO: change to std::unique_ptr<PartitionValueId[]>
using LTPVComparisonOrderedRTPValueIDs = std::vector<PartitionValueId>;

struct LTPValueUpperSetMapping {
    LTPVComparisonOrderedRTPValueIDs rt_pvalue_ids;
    LTPValueRCVIDUpperSetCardinalityMap cardinality_map;

    MaterializedPValuesUpperSet GetUpperSet(RecordClassifierValueId rcv_id) const {
        auto it = cardinality_map.lower_bound(rcv_id);
        if (it == cardinality_map.end()) return {0, {}};
        auto const& [record_set_cardinality, value_set_cardinality] = it->second;
        DESBORDANTE_ASSUME(record_set_cardinality != 0);
        return {record_set_cardinality, {&rt_pvalue_ids.front(), value_set_cardinality}};
    }
};

using UpperSetIndex = std::vector<LTPValueUpperSetMapping>;
// NOTE:
// upper set operations required for the algorithm:
// - iterate through records
// - check membership
// - get number of records
}  // namespace algos::hymde::record_match_indexes
