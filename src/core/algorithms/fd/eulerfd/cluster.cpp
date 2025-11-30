#include "core/algorithms/fd/eulerfd/cluster.h"

#include <cstdint>

namespace algos {

// We want to have the same permutation of array on all platforms and compilers
// (it is necessary for consistent hash values during testing).
// But the implementation of std::shuffle depends on STL, so we can't use it.
void Cluster::ShuffleData(RandomStrategy const& custom_rand) {
    if (cluster_data_.empty()) {
        return;
    }
    for (size_t i = cluster_data_.size() - 1; i > 0; i--) {
        std::swap(cluster_data_[i], cluster_data_[custom_rand() % (i + 1)]);
    }
}

Cluster::Cluster(std::vector<size_t>&& cluster_data, RandomStrategy const& custom_rand)
    : cluster_data_(std::move(cluster_data)) {
    ShuffleData(custom_rand);
    hist_effects_.fill(1);
}

double Cluster::GetAverage() const {
    double const sum = std::accumulate(hist_effects_.begin(), hist_effects_.end(), 0.0);
    return sum / kInitialWindow;
}

double Cluster::Sample(RegisterTuplesFunction const& handle_tuples) {
    new_tuples_pairs_ = new_non_fds_ = 0;
    window_++;

    int64_t barrier = static_cast<int64_t>(cluster_data_.size()) - static_cast<int64_t>(window_);
    for (int64_t i = 0; i < barrier; i++) {
        new_non_fds_ += handle_tuples(cluster_data_[i], cluster_data_[i + window_]);
    }
    new_tuples_pairs_ = cluster_data_.size() - window_;

    double cur_eff =
            new_tuples_pairs_ == 0 ? 0 : (static_cast<double>(new_non_fds_) / new_tuples_pairs_);
    hist_effects_[sample_number_ % kInitialWindow] = cur_eff;

    sample_number_++;

    return cur_eff;
}

}  // namespace algos
