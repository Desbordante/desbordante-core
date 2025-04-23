#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "algorithms/mde/hymde/record_identifier.h"

namespace algos::hymde::indexes {
// class that allows: iterating, membership checking, getting length
// Python-style iteration??

// used in validation and sampling
class IUpperSet {
public:
    // length
    virtual std::size_t Size() const = 0;
    // contains
    virtual bool Contains(RecordIdentifier record) const = 0;
    // get intersection
    virtual std::vector<RecordIdentifier> IntersectWith(std::span<IUpperSet const*> sets) const = 0;
    // some method to implement sampling
    virtual void DumpElements(RecordIdentifier* start) const = 0;
};

}  // namespace algos::hymde::indexes
