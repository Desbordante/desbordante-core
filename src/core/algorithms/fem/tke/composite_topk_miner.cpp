#include "core/algorithms/fem/tke/composite_topk_miner.h"

namespace algos::tke {

CompositeTopKMiner::CompositeTopKMiner(size_t k, size_t window_length)
    : k_(k), window_length_(window_length) {}

void CompositeTopKMiner::TryAdd(CompositeEpisode ep, TopK& top_k, Explore& explore,
                                size_t floor_minsup) const {
    size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : floor_minsup;
    if (ep.GetSupport() < minsup) {
        return;
    }
    explore.push(ep);
    if (top_k.size() < k_) {
        top_k.push(std::move(ep));
    } else if (ep.GetSupport() > top_k.top().GetSupport()) {
        top_k.pop();
        top_k.push(std::move(ep));
    }
}

std::vector<CompositeEpisode> CompositeTopKMiner::Mine(
        std::vector<ParallelEpisode>&& parallel_episodes, size_t initial_minsup) {
    if (parallel_episodes.empty()) {
        return {};
    }

    TopK top_k;
    Explore explore;

    for (ParallelEpisode const& p : parallel_episodes) {
        TryAdd(CompositeEpisode(p), top_k, explore, initial_minsup);
    }

    while (!explore.empty()) {
        CompositeEpisode item = std::move(const_cast<CompositeEpisode&>(explore.top()));
        explore.pop();

        size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : initial_minsup;
        if (item.GetSupport() < minsup) {
            continue;
        }

        for (ParallelEpisode const& ext : parallel_episodes) {
            if (ext.GetSupport() < minsup) {
                continue;
            }
            std::optional<CompositeEpisode> child = item.TryExtend(ext, minsup, window_length_);
            if (child) {
                TryAdd(std::move(*child), top_k, explore, initial_minsup);
            }
        }
    }

    std::vector<CompositeEpisode> result;
    result.reserve(top_k.size());
    while (!top_k.empty()) {
        result.push_back(std::move(const_cast<CompositeEpisode&>(top_k.top())));
        top_k.pop();
    }
    return result;
}

}  // namespace algos::tke
