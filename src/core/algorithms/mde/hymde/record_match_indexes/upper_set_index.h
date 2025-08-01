#pragma once

#include <span>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_identifier.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "model/index.h"
#include "util/desbordante_assume.h"

namespace algos::hymde::record_match_indexes {
// highest comparison value first
// TODO: change to std::unique_ptr<RecordIdentifier[]>
using OrderedValues = std::vector<PartitionValueId>;
// When OrderedRecords is changed to be a bare pointer, std::greater will make things easier: the
// pointer that gets passed to free and the end pointer will have to be stored somewhere either way,
// this way the former won't have to be pulled out of the map.
//

struct RecordSetInfo {
    std::size_t number_of_records;
    model::Index end_index;
};

struct PartValueSet {
    std::size_t number_of_records;
    std::span<PartitionValueId const> values;

    bool IsEmpty() const noexcept {
        return number_of_records == 0;
    }
};

using EndIdMapTemp = boost::container::flat_map<RecordClassifierValueId, RecordSetInfo>;

struct FlatUpperSetIndex {
    OrderedValues values;
    EndIdMapTemp end_ids_temp;

    PartValueSet GetMatchedValues(RecordClassifierValueId rcv_id) const {
        auto it = end_ids_temp.lower_bound(rcv_id);
        if (it == end_ids_temp.end()) return {0, {}};
        auto const& [number_of_records, end_index] = it->second;
        DESBORDANTE_ASSUME(number_of_records != 0);
        return {number_of_records, {&values.front(), end_index}};
    }
};

// How to check membership in upper set:
// val_matrices[rm_index][left_pvalue][right_records[right_record_id][rm_index]] >= rcv_id
// upper set operations: iterate through records, check membership, get number of records
using UpperSetIndex = std::vector<FlatUpperSetIndex>;
}  // namespace algos::hymde::record_match_indexes
