#include "pattern.h"

#include <boost/dynamic_bitset.hpp>

namespace algos::cfdfinder {
bool Pattern::Matches(algos::hy::Row const& tuple) const {
    for (auto const& [id, entry] : entries_) {
        if (!entry->Matches(tuple[id])) return false;
    }
    return true;
}

void Pattern::UpdateCover(Pattern const& pattern) {
    std::unordered_set<int> clusters_elements;
    clusters_elements.reserve(pattern.GetNumCover());

    for (auto const& pcluster : pattern.GetCover()) {
        clusters_elements.insert(pcluster.begin(), pcluster.end());
    }

    for (auto& cluster : cover_) {
        std::erase_if(cluster, [&clusters_elements](int element) {
            return clusters_elements.contains(element);
        });

        if (PatternDebugController::IsDebugEnabled()) {
            std::sort(cluster.begin(), cluster.end());
        }
    }

    std::erase_if(cover_, [](auto const& c) { return c.empty(); });
    if (PatternDebugController::IsDebugEnabled()) {
        cover_.sort([](auto const& a, auto const& b) { return a.front() < b.front(); });
    }

    support_ = static_cast<double>(GetNumCover());
}

void Pattern::UpdateKeepers(algos::hy::Row const& inverted_pli_rhs) {
    size_t child_violations = std::accumulate(
            cover_.begin(), cover_.end(), 0u,
            [&inverted_pli_rhs](size_t sum, Cluster const& cluster) {
                return sum + algos::cfdfinder::util::CalculateViolations(cluster, inverted_pli_rhs);
            });

    num_keepers_ = GetNumCover() - child_violations;
}

size_t Pattern::GetNumCover() const {
    return std::accumulate(cover_.begin(), cover_.end(), 0u,
                           [](size_t sum, Cluster const& cluster) { return sum + cluster.size(); });
}

bool Pattern::operator<(Pattern const& other) const noexcept {
    if (other.support_ != support_) {
        return other.support_ > support_;
    }
    if (PatternDebugController::IsDebugEnabled()) {
        if (other.num_keepers_ != num_keepers_) {
            return other.num_keepers_ > num_keepers_;
        }
        return other.number_ > number_;  // debug
    }

    return other.num_keepers_ > num_keepers_;
}

}  // namespace algos::cfdfinder