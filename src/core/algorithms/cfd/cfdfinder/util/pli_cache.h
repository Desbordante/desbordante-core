#pragma once

#include <memory>

#include "model/table/position_list_index.h"
#include "model/table/vertical_map.h"
#include "types/bitset.h"

namespace algos::cfdfinder {
class PLICache {
private:
    using PLIMap = model::VerticalMap<model::PLI>;

    std::unique_ptr<PLIMap> index_;
    RelationalSchema const* schema_;
    size_t limit_;
    size_t size_;
    size_t full_hits_;
    size_t partial_hits_;
    size_t total_misses_;

public:
    PLICache(size_t limit, RelationalSchema const* schema)
        : index_(std::make_unique<PLIMap>(schema)),
          schema_(schema),
          limit_(limit),
          size_(0),
          full_hits_(0),
          partial_hits_(0),
          total_misses_(0) {}

    std::shared_ptr<model::PLI const> Get(BitSet const& columns) const {
        return index_->Get(columns);
    }

    void Put(BitSet columns, std::shared_ptr<model::PLI> pli) {
        if (size_ >= limit_) {
            return;
        }
        index_->Put(Vertical(schema_, std::move(columns)), std::move(pli));
        size_++;
    }

    void AddFullHit() {
        full_hits_++;
    }

    void AddPartialHit() {
        partial_hits_++;
    }

    void AddMiss() {
        total_misses_++;
    }

    size_t GetFullHits() const {
        return full_hits_;
    }

    size_t GetPartialHits() const {
        return partial_hits_;
    }

    size_t GetTotalMisses() const {
        return total_misses_;
    }
};
}  // namespace algos::cfdfinder