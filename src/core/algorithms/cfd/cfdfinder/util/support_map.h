#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <variant>

#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/config/thread_number/type.h"

namespace algos::cfdfinder::utils {
class SupportMap {
private:
    class ThreadSafeSupportMap {
    private:
        std::unordered_map<Candidate, double> support_map_;
        mutable std::shared_mutex mutex_;

    public:
        void SetSupport(Candidate const& candidate, double support) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            support_map_[candidate] = support;
        }

        double GetSupport(Candidate const& candidate) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = support_map_.find(candidate);
            if (it != support_map_.end()) {
                return it->second;
            }
            return 0.0;
        }

        bool Contains(Candidate const& candidate) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return support_map_.contains(candidate);
        }
    };

    using MapContainer = std::variant<std::unordered_map<Candidate, double>,
                                      std::shared_ptr<ThreadSafeSupportMap>>;

    MapContainer support_map_;

public:
    SupportMap(config::ThreadNumType thread_num) {
        if (thread_num > 1) {
            support_map_ = std::make_shared<ThreadSafeSupportMap>();
        } else {
            support_map_ = std::unordered_map<Candidate, double>();
        }
    };

    void SetSupport(Candidate const& candidate, double support);
    double GetSupport(Candidate const& candidate) const;
    bool Contains(Candidate const& candidate) const;
};

}  // namespace algos::cfdfinder::utils