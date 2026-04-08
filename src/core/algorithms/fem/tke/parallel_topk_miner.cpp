#include "core/algorithms/fem/tke/parallel_topk_miner.h"

namespace algos::tke {

ParallelTopKMiner::ParallelTopKMiner(model::Event events_num, size_t k,
                                     std::vector<std::shared_ptr<LocationList>> events_loc_lists)
    : events_num_(events_num), k_(k), events_loc_lists_(std::move(events_loc_lists)) {}

void ParallelTopKMiner::TryAdd(ParallelEpisode ep, TopK& top_k, Explore& explore) const {
    size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : 1;
    if (ep.GetSupport() < minsup) {
        return;
    }
    if (top_k.size() < k_) {
        top_k.push(ep);
    } else if (ep.GetSupport() > minsup) {
        top_k.pop();
        top_k.push(ep);
    }
    explore.push(std::move(ep));
}

std::vector<ParallelEpisode> ParallelTopKMiner::Mine() {
    TopK top_k;
    Explore explore;

    for (ParallelEpisode& seed :
         ParallelEpisode::BuildParallelEpisodesWithEvents(events_loc_lists_, events_num_)) {
        TryAdd(std::move(seed), top_k, explore);
    }

    while (!explore.empty()) {
        ParallelEpisode item = std::move(const_cast<ParallelEpisode&>(explore.top()));
        explore.pop();

        size_t const minsup = (top_k.size() == k_) ? top_k.top().GetSupport() : 1;
        if (item.GetSupport() < minsup) {
            continue;
        }

        for (model::Event event = item.GetLastEvent() + 1; event < events_num_; ++event) {
            TryAdd(item.ParallelExtension(event, *events_loc_lists_[event]), top_k, explore);
        }
    }

    std::vector<ParallelEpisode> result;
    result.reserve(top_k.size());
    while (!top_k.empty()) {
        result.push_back(std::move(const_cast<ParallelEpisode&>(top_k.top())));
        top_k.pop();
    }
    return result;
}

}  // namespace algos::tke
