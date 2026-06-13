#include "core/algorithms/cfd/cfdfinder/util/pli_cache.h"

namespace algos::cfdfinder {
void PLICache::Put(boost::dynamic_bitset<> columns, std::shared_ptr<model::PLI> pli) {
    if (size_ >= limit_) {
        return;
    }
    index_->Put(Vertical(schema_, std::move(columns)), std::move(pli));
    ++size_;
}

std::shared_ptr<model::PLI const> PLICache::GetLhsPli(boost::dynamic_bitset<> const& lhs) {
    if (auto cached = index_->Get(lhs)) {
        ++full_hits_;
        return cached;
    }

    boost::dynamic_bitset<> current_lhs(lhs.size());
    std::shared_ptr<model::PLI const> result;

    util::ForEachIndex(lhs, [&](size_t index) {
        current_lhs.flip(index);

        if (auto cached = index_->Get(current_lhs)) {
            ++partial_hits_;
            result = std::move(cached);
            return;
        }

        if (!result) {
            result = std::shared_ptr<model::PLI const>((*plis_)[index], [](model::PLI*) {});
            return;
        }

        ++total_misses_;

        auto intersection = std::shared_ptr<model::PLI>(result->Intersect((*plis_)[index]));

        Put(current_lhs, intersection);

        result = index_->Get(current_lhs);
        if (!result) {
            result = intersection;
        }
    });

    return result;
}
}  // namespace algos::cfdfinder