#include "core/algorithms/fd/eulerfd/mlfq.h"

#include <cmath>

namespace algos {

bool MLFQ::LastQueueElement::operator<(LastQueueElement const &other) const {
    return cluster->GetAverage() < other.cluster->GetAverage();
}

MLFQ::MLFQ(size_t queues_number) {
    double range = kLastQueueRangeBarrier;
    queues_.reserve(queues_number);
    for (size_t i = 0; i < queues_number; i++) {
        queues_.push_back({{}, range});
        range *= 10;
    }
}

void MLFQ::Add(Cluster *cluster, double priority, bool add_if_zero) {
    if (priority == 0 && !add_if_zero) {
        return;
    }
    if (priority < kLastQueueRangeBarrier) {
        AddAtLast(cluster);
    } else {
        // Evaluating index of queue
        int queue = static_cast<int>(std::floor(std::log10(priority))) + 3;
        queue = queue > 4 ? 4 : queue;
        actual_queue_ = std::max(actual_queue_, queue);
        queues_[queue].first.push(cluster);
        effective_size_++;
    }
}

void MLFQ::AddAtLast(Cluster *cluster) {
    if (cluster->GetAverage() > 0) {
        last_queue_.push({cluster});
    }
}

Cluster *MLFQ::Get() {
    if (actual_queue_ >= 0) {
        Cluster *save = queues_[actual_queue_].first.front();
        queues_[actual_queue_].first.pop();
        effective_size_--;
        while (actual_queue_ >= 0 && queues_[actual_queue_].first.empty()) {
            actual_queue_--;
        }
        return save;
    }
    Cluster *cluster = last_queue_.top().cluster;
    last_queue_.pop();
    return cluster;
}

size_t MLFQ::GetEffectiveSize() const {
    return effective_size_;
}

size_t MLFQ::GetLastQueueSize() const {
    return last_queue_.size();
}

double MLFQ::MaxEffectInLastQueue() const {
    return last_queue_.top().cluster->GetAverage();
}

void MLFQ::Clear() {
    last_queue_ = {};
    for (auto &q : queues_) {
        q.first = {};
    }
}
}  // namespace algos
