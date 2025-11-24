#include "algorithms/cfd/cfdfinder/model/pattern/pattern.h"

#include <algorithm>
#include <numeric>
#include <tuple>
#include <unordered_set>

#include "algorithms/cfd/cfdfinder/util/violations_util.h"

namespace algos::cfdfinder {
bool Pattern::Matches(Row const& tuple) const {
    for (auto const& [id, entry] : entries_) {
        if (!entry->Matches(tuple[id])) {
            return false;
        }
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
            std::ranges::sort(cluster);
        }
    }

    std::erase_if(cover_, [](auto const& c) { return c.empty(); });
    if (PatternDebugController::IsDebugEnabled()) {
        cover_.sort([](auto const& a, auto const& b) { return a.front() < b.front(); });
    }

    support_ = static_cast<double>(GetNumCover());
}

void Pattern::UpdateKeepers(Row const& inverted_pli_rhs) {
    size_t child_violations =
            std::accumulate(cover_.begin(), cover_.end(), 0u,
                            [&inverted_pli_rhs](size_t sum, Cluster const& cluster) {
                                return sum + algos::cfdfinder::utils::CalculateViolations(
                                                     cluster, inverted_pli_rhs);
                            });

    num_keepers_ = GetNumCover() - child_violations;
}

size_t Pattern::GetNumCover() const {
    return std::accumulate(cover_.begin(), cover_.end(), 0u,
                           [](size_t sum, Cluster const& cluster) { return sum + cluster.size(); });
}

bool Pattern::operator<(Pattern const& other) const noexcept {
    if (PatternDebugController::IsDebugEnabled()) {
        return std::tie(support_, num_keepers_, number_) <
               std::tie(other.support_, other.num_keepers_, other.number_);
    } else {
        return std::tie(support_, num_keepers_) < std::tie(other.support_, other.num_keepers_);
    }
}
}  // namespace algos::cfdfinder
