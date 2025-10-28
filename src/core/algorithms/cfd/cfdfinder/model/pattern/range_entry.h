#pragma once

#include <vector>

#include <boost/functional/hash.hpp>

#include "entry.h"

namespace algos::cfdfinder {
class RangeEntry final : public Entry {
    std::vector<size_t> sorted_cluster_ids_;
    size_t min_cluster_;
    size_t max_cluster_;

public:
    RangeEntry(std::vector<size_t> const& sorted_clusters, size_t min_cluster, size_t max_cluster)
        : sorted_cluster_ids_(sorted_clusters),
          min_cluster_(min_cluster),
          max_cluster_(max_cluster) {}

    bool IncreaseLowerBound() {
        min_cluster_++;
        return min_cluster_ <= max_cluster_;
    }

    bool DecreaseUpperBound() {
        if (max_cluster_ == 0) {
            return max_cluster_ >= (min_cluster_ + 1);
        }
        max_cluster_--;
        return max_cluster_ >= min_cluster_;
    }

    std::shared_ptr<Entry> Clone() const {
        return std::make_shared<RangeEntry>(*this);
    }

    size_t GetLowerBound() const {
        return sorted_cluster_ids_[min_cluster_];
    }

    size_t GetUpperBound() const {
        return sorted_cluster_ids_[max_cluster_];
    }

    inline bool Matches(size_t value) const final override {
        return value >= min_cluster_ && value <= max_cluster_;
    }

    bool operator==(Entry const& other) const override {
        if (other.GetType() != EntryType::kRange) return false;
        auto const* entry = static_cast<RangeEntry const*>(&other);

        return min_cluster_ == entry->min_cluster_ && max_cluster_ == entry->max_cluster_ &&
               sorted_cluster_ids_ == entry->sorted_cluster_ids_;
    }

    size_t Hash() const override {
        size_t hash = boost::hash_range(sorted_cluster_ids_.begin(), sorted_cluster_ids_.end());
        hash += std::hash<size_t>{}(min_cluster_);
        hash += std::hash<size_t>{}(max_cluster_);

        return hash;
    }

    EntryType GetType() const override {
        return EntryType::kRange;
    }
};
}  // namespace algos::cfdfinder
