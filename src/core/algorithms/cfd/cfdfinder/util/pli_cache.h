#pragma once

#include <cstddef>
#include <memory>

#include <boost/dynamic_bitset.hpp>

#include "core/model/table/position_list_index.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/vertical_map.h"

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

    std::shared_ptr<model::PLI const> Get(boost::dynamic_bitset<> const& columns) const {
        return index_->Get(columns);
    }

    void Put(boost::dynamic_bitset<> columns, std::shared_ptr<model::PLI> pli) {
        if (size_ >= limit_) {
            return;
        }
        index_->Put(Vertical(schema_, std::move(columns)), std::move(pli));
        ++size_;
    }

    void AddFullHit() {
        ++full_hits_;
    }

    void AddPartialHit() {
        ++partial_hits_;
    }

    void AddMiss() {
        ++total_misses_;
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
