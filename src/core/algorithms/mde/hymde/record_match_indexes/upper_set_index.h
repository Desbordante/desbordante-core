#pragma once

#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_identifier.h"
#include "model/index.h"

namespace algos::hymde::record_match_indexes {
// highest comparison value first
// TODO: change to std::unique_ptr<RecordIdentifier[]>
using OrderedRecords = std::vector<RecordIdentifier>;
// When OrderedRecords is changed to be a bare pointer, std::greater will make things easier: the
// pointer that gets passed to free and the end pointer will have to be stored somewhere either way,
// this way the former won't have to be pulled out of the map.
using EndIdMap = boost::container::flat_map<RecordClassifierValueId, model::Index, std::greater<>>;

struct FlatUpperSetIndex {
    OrderedRecords records;
    EndIdMap end_ids;
};

// TODO: remove the need for this, waste of memory
// How to check membership in upper set:
// sim_matrices[col_match_index][left_value][right_records[right_record_id][col_mathes
// [col_match_index].right_column_index]] >= sim
using UpperSet = boost::unordered::unordered_flat_set<RecordIdentifier>;
using UpperSetMapping = boost::container::flat_map<RecordClassifierValueId, UpperSet>;

class FastUpperSetMapping {
    FlatUpperSetIndex flat_;
    UpperSetMapping sets_;

public:
    FlatUpperSetIndex const& GetFlat() const noexcept {
        return flat_;
    }

    UpperSet const* GetUpperSet(RecordClassifierValueId lhs_ccv_id) const {
        auto it = sets_.lower_bound(lhs_ccv_id);
        if (it == sets_.end()) return nullptr;
        return &it->second;
    }

    FastUpperSetMapping(FlatUpperSetIndex flat);
    FastUpperSetMapping() = default;
};

using ValueUpperSetMapping = FastUpperSetMapping;
using UpperSetIndex = std::vector<ValueUpperSetMapping>;
}  // namespace algos::hymde::record_match_indexes
