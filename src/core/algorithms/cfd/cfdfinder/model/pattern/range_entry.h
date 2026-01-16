#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <boost/functional/hash.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"

namespace algos::cfdfinder {
class RangeEntry final : public Entry {
    std::shared_ptr<std::vector<size_t>> sorted_cluster_ids_;
    size_t min_cluster_;
    size_t max_cluster_;

public:
    RangeEntry(std::shared_ptr<std::vector<size_t>> sorted_clusters, size_t min_cluster,
               size_t max_cluster)
        : sorted_cluster_ids_(std::move(sorted_clusters)),
          min_cluster_(min_cluster),
          max_cluster_(max_cluster) {}

    bool IncreaseLowerBound() {
        ++min_cluster_;
        return min_cluster_ <= max_cluster_;
    }

    bool DecreaseUpperBound() {
        if (max_cluster_ == 0) {
            return false;
        }
        --max_cluster_;
        return min_cluster_ <= max_cluster_;
    }

    std::shared_ptr<Entry> Clone() const {
        return std::make_shared<RangeEntry>(*this);
    }

    inline bool Matches(size_t value) const final override {
        return value >= min_cluster_ && value <= max_cluster_;
    }

    bool IsConstant() const override {
        return false;
    }

    bool operator==(Entry const& other) const override {
        auto const* entry = dynamic_cast<RangeEntry const*>(&other);

        return entry != nullptr && min_cluster_ == entry->min_cluster_ &&
               max_cluster_ == entry->max_cluster_ &&
               sorted_cluster_ids_ == entry->sorted_cluster_ids_;
    }

    bool operator!=(Entry const& other) const {
        return !(*this == other);
    }

    size_t Hash() const override {
        size_t hash = 0;

        boost::hash_range(hash, sorted_cluster_ids_->begin(), sorted_cluster_ids_->end());
        boost::hash_combine(hash, min_cluster_);
        boost::hash_combine(hash, max_cluster_);

        return hash;
    }

    std::string ToString(InvertedClusterMap const& cluster_map) const override {
        if (min_cluster_ == 0 && max_cluster_ + 1 == sorted_cluster_ids_->size()) {
            return std::string(kWildCard);
        }
        std::string lower_bound = cluster_map.at((*sorted_cluster_ids_)[min_cluster_]);

        if (min_cluster_ == max_cluster_) {
            return lower_bound;
        }
        std::string upper_bound = cluster_map.at((*sorted_cluster_ids_)[max_cluster_]);
        return "[" + (!lower_bound.empty() ? lower_bound : std::string(kNullRepresentation)) +
               " - " + (!upper_bound.empty() ? upper_bound : std::string(kNullRepresentation)) +
               "]";
    }
};
}  // namespace algos::cfdfinder
