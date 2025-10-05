#pragma once

#include <algorithm>
#include <numeric>

#include "cfd/cfdfinder/util/violations_util.h"
#include "pattern_item.h"

namespace algos::cfdfinder {

class PatternDebugController {
private:
    inline static bool debug_enabled_ = false;
    inline static unsigned long long counter = 0;

public:
    static bool IsDebugEnabled() {
        return debug_enabled_;
    }

    static void SetDebugEnabled(bool enabled) {
        debug_enabled_ = enabled;
    }

    static void ResetCounter() {
        counter = 0;
    }

    static unsigned long long Next() {
        return counter++;
    }
};

class Pattern {
private:
    Entries entries_;
    std::list<Cluster> cover_;
    double support_;
    size_t num_keepers_;
    unsigned long long number_;

public:
    explicit Pattern(Entries&& entries) : entries_(std::move(entries)) {
        if (PatternDebugController::IsDebugEnabled()) {
            number_ = PatternDebugController::Next();
        }
    }

    Pattern(Pattern&& other) noexcept = default;
    Pattern(Pattern const& other) = default;
    Pattern& operator=(Pattern&& other) noexcept = default;
    Pattern& operator=(Pattern const& other) = default;

    bool operator<(Pattern const& other) const noexcept;

    bool operator==(Pattern const& other) const {
        return entries_ == other.entries_;
    };

    bool Matches(algos::hy::Row const& tuple) const;
    void UpdateCover(Pattern const& pattern);
    void UpdateKeepers(algos::hy::Row const& inverted_pli_rhs);
    size_t GetNumCover() const;

    Entries const& GetEntries() const {
        return entries_;
    }

    double GetSupport() const {
        return support_;
    }

    void SetSupport(double support) {
        support_ = support;
    }

    double GetConfidence() const {
        auto num_cover = GetNumCover();
        return num_cover == 0 ? 0
                              : static_cast<double>(num_keepers_) / static_cast<double>(num_cover);
    }

    std::list<Cluster> const& GetCover() const {
        return cover_;
    }

    void ClearCover() {
        cover_.clear();
    }

    void SetCover(std::list<Cluster>&& new_cover) {
        cover_ = std::move(new_cover);

        if (PatternDebugController::IsDebugEnabled()) {
            for (auto& cluster : cover_) {
                std::sort(cluster.begin(), cluster.end());
            }
            cover_.sort([](Cluster const& a, Cluster const& b) { return a.front() < b.front(); });
        }
    }

    size_t GetNumKeepers() const {
        return num_keepers_;
    }

    void SetNumKeepers(size_t num_keepers) {
        num_keepers_ = num_keepers;
    }
};

}  // namespace algos::cfdfinder

template <>
struct std::hash<algos::cfdfinder::Entries> {
    size_t operator()(algos::cfdfinder::Entries const& entries) const {
        size_t seed = 0;

        boost::hash_combine(seed, entries.size());

        for (auto const& [id, entry] : entries) {
            boost::hash_combine(seed, id);
            boost::hash_combine(seed, entry->Hash());
        }

        return seed;
    }
};

template <>
struct std::hash<algos::cfdfinder::Pattern> {
    size_t operator()(algos::cfdfinder::Pattern const& p) const {
        return std::hash<algos::cfdfinder::Entries>{}(p.GetEntries());
    }
};