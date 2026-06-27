#pragma once

#include <cstddef>
#include <tuple>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_item.h"
#include "core/algorithms/cfd/cfdfinder/types/cluster.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {

class Pattern {
private:
    Entries const entries_;
    std::vector<Cluster> cover_;
    double support_;
    size_t num_keepers_;
    size_t const cached_hash_;  // To avoid recomputation during hash lookups and bucket
                                // redistribution.

    size_t CalculateViolations(Row const& inverted_rhs_pli) const;

public:
    explicit Pattern(Entries&& entries)
        : entries_(std::move(entries)), cached_hash_(std::hash<Entries>{}(entries_)) {}

    Pattern(Entries&& entries, std::vector<Cluster>&& cover, Row const& inverted_pli_rhs)
        : entries_(std::move(entries)),
          cover_(std::move(cover)),
          support_(GetNumCover()),
          cached_hash_(std::hash<Entries>{}(entries_)) {
        UpdateKeepers(inverted_pli_rhs);
    }

    size_t GetHash() const noexcept {
        return cached_hash_;
    }

    bool operator==(Pattern const& other) const noexcept {
        return entries_ == other.entries_;
    }

    bool operator<(Pattern const& other) const noexcept {
        return std::tie(support_, num_keepers_, other.entries_) <
               std::tie(other.support_, other.num_keepers_, entries_);
    }

    bool operator>(Pattern const& other) const noexcept {
        return other < *this;
    }

    void UpdateCover(boost::dynamic_bitset<> const& used_rows);
    void UpdateKeepers(Row const& inverted_pli_rhs);

    size_t GetNumCover() const;

    Entries const& GetEntries() const noexcept {
        return entries_;
    }

    double GetSupport() const noexcept {
        return support_;
    }

    double GetConfidence() const {
        auto num_cover = GetNumCover();
        return num_cover == 0 ? 0 : static_cast<double>(num_keepers_) / num_cover;
    }

    std::vector<Cluster> const& GetCover() const noexcept {
        return cover_;
    }

    size_t GetNumKeepers() const noexcept {
        return num_keepers_;
    }
};

}  // namespace algos::cfdfinder

template <>
struct std::hash<algos::cfdfinder::Pattern> {
    size_t operator()(algos::cfdfinder::Pattern const& p) const {
        return p.GetHash();
    }
};
