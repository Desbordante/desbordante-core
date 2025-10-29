#pragma once

#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "cluster.h"

namespace algos {

class MLFQ {
private:
    using Queue = std::pair<std::queue<Cluster *>, double>;

    constexpr static double kLastQueueRangeBarrier = 0.001;

    struct LastQueueElement {
        Cluster *cluster{};
        bool operator<(LastQueueElement const &other) const;
    };

    std::vector<Queue> queues_;
    size_t effective_size_ = 0;
    std::priority_queue<LastQueueElement> last_queue_{};

    // Index of not empty queue with the highest priority
    int actual_queue_ = -1;

public:
    explicit MLFQ(size_t queues_number);

    void Add(Cluster *cluster, double priority, bool add_if_zero = false);
    void AddAtLast(Cluster *cluster);
    [[nodiscard]] Cluster *Get();

    [[nodiscard]] size_t GetEffectiveSize() const;
    [[nodiscard]] size_t GetLastQueueSize() const;
    [[nodiscard]] double MaxEffectInLastQueue() const;
    void Clear();
};
}  // namespace algos
