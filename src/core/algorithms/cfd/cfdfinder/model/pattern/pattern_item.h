#pragma once
#include <memory>
#include <vector>

#include "entry.h"

namespace algos::cfdfinder {
struct PatternItem {
    size_t id;
    std::shared_ptr<Entry> entry;

    bool operator==(PatternItem const& other) const {
        return id == other.id && *entry == *(other.entry);
    }
};

using Entries = std::vector<PatternItem>;
}  // namespace algos::cfdfinder