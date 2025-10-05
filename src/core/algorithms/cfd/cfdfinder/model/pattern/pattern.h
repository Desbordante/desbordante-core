#pragma once

#include <compare>
#include <cstddef>
#include <list>
#include <ranges>

#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_item.h"
#include "core/algorithms/cfd/cfdfinder/types/cluster.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {

class Pattern {
private:
    Entries entries_;
    std::list<Cluster> cover_;
    double support_;
    size_t num_keepers_;

public:
    explicit Pattern(Entries&& entries) : entries_(std::move(entries)) {}

    Pattern(Pattern&& other) noexcept = default;
    Pattern(Pattern const& other) = default;
    Pattern& operator=(Pattern&& other) noexcept = default;
    Pattern& operator=(Pattern const& other) = default;

    bool operator<(Pattern const& other) const noexcept {
        return std::tie(support_, num_keepers_, other.entries_) <
               std::tie(other.support_, other.num_keepers_, entries_);
    }

    bool operator==(Pattern const& other) const noexcept {
        return entries_ == other.entries_;
    };

    bool operator!=(Pattern const& other) const {
        return !(*this == other);
    }

    bool operator>(Pattern const& other) const noexcept {
        return other < *this;
    }

    bool operator<=(Pattern const& other) const noexcept {
        return !(other < *this);
    }

    bool operator>=(Pattern const& other) const noexcept {
        return !(*this < other);
    }

    bool Matches(Row const& tuple) const;
    void UpdateCover(Pattern const& pattern);
    void UpdateKeepers(Row const& inverted_pli_rhs);
    size_t GetNumCover() const;

    Entries const& GetEntries() const noexcept {
        return entries_;
    }

    double GetSupport() const noexcept {
        return support_;
    }

    void SetSupport(double support) {
        support_ = support;
    }

    double GetConfidence() const {
        auto num_cover = GetNumCover();
        return num_cover == 0 ? 0 : static_cast<double>(num_keepers_) / num_cover;
    }

    std::list<Cluster> const& GetCover() const noexcept {
        return cover_;
    }

    void SetCover(std::list<Cluster>&& new_cover) {
        cover_ = std::move(new_cover);
    }

    size_t GetNumKeepers() const noexcept {
        return num_keepers_;
    }

    void SetNumKeepers(size_t num_keepers) {
        num_keepers_ = num_keepers;
    }
};

}  // namespace algos::cfdfinder

template <>
struct std::hash<algos::cfdfinder::Pattern> {
    size_t operator()(algos::cfdfinder::Pattern const& p) const {
        return std::hash<algos::cfdfinder::Entries>{}(p.GetEntries());
    }
};
