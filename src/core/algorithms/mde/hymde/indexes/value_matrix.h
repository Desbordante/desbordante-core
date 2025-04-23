#pragma once

#include <boost/unordered/unordered_flat_map.hpp>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "util/desbordante_assume.h"

namespace algos::hymde::indexes {
// part val, part val -> comp. val
// approaches:
// - store all results, get with dict (PrecalculatedValueMatrix)
// - store all values, calculate on the fly (CalculatingValueMatrix)
// - store all values, cache some results (CachedValueMatrix)
class IValueMatrix {
public:
    // get RCV ID given two values
    // none of the values involved have to be indices
    // returns kLowestRCValueId if values cannot be matched using a non-total decision boundary
    virtual RecordClassifierValueId GetRCVId(PartitionValueId left,
                                             PartitionValueId right) const = 0;
};

class PrecalculatedValueMatrix : IValueMatrix {
    using Row = boost::unordered::unordered_flat_map<PartitionValueId, RecordClassifierValueId>;

    std::vector<Row> rows_;

public:
    RecordClassifierValueId GetRCVId(PartitionValueId left, PartitionValueId right) const final {
        Row const& row = rows_[left];

        auto const it = row.find(right);
        if (it == row.end()) return kLowestRCValueId;
        DESBORDANTE_ASSUME(it->second != kLowestRCValueId);
        return it->second;
    };
};

}  // namespace algos::hymde::indexes
