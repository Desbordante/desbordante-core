#pragma once

#include <memory>
#include <vector>

#include "algorithms/mde/hymde/indexes/upper_set.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::indexes {
class IValueUpperSetIndex {
public:
    virtual IUpperSet* GetUpperSet(RecordClassifierValueId rcv_id) = 0;
};

class UpperSetIndex {
    std::vector<std::unique_ptr<IValueUpperSetIndex>> sets;

public:
    // given left value and RCV ID, get upper set
    IUpperSet* GetUpperSet(PartitionValueId left, RecordClassifierValueId rcv_id) {
        return sets[left]->GetUpperSet(rcv_id);
    }
};
}  // namespace algos::hymde::indexes
