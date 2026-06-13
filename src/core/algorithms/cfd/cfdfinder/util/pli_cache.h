#pragma once

#include <cstddef>
#include <memory>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"
#include "core/model/table/position_list_index.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/vertical_map.h"

namespace algos::cfdfinder {
class PLICache {
private:
    using PLIMap = model::VerticalMap<model::PLI>;

    std::unique_ptr<PLIMap> index_;
    RelationalSchema const* schema_;
    PLIsPtr const plis_;
    size_t limit_;
    size_t size_ = 0;
    size_t full_hits_ = 0;
    size_t partial_hits_ = 0;
    size_t total_misses_ = 0;

    void Put(boost::dynamic_bitset<> columns, std::shared_ptr<model::PLI> pli);

public:
    PLICache(size_t limit, RelationalSchema const* schema, PLIsPtr plis)
        : index_(std::make_unique<PLIMap>(schema)), schema_(schema), plis_(plis), limit_(limit) {}

    std::shared_ptr<model::PLI const> GetLhsPli(boost::dynamic_bitset<> const& lhs);

    size_t GetFullHits() const noexcept {
        return full_hits_;
    }

    size_t GetPartialHits() const noexcept {
        return partial_hits_;
    }

    size_t GetTotalMisses() const noexcept {
        return total_misses_;
    }
};
}  // namespace algos::cfdfinder
