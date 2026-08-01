#pragma once
#include <functional>
#include <list>
#include <mutex>

#include "core/algorithms/fd/pyrocommon/model/partial_fd.h"
#include "core/algorithms/fd/pyrocommon/model/partial_key.h"

class DependencyConsumer {
private:
    std::mutex mutable discover_fd_mutex_;
    std::mutex mutable discover_ucc_mutex_;

    std::list<PartialFD> discovered_fds_;
    std::list<PartialKey> discovered_uccs_;

protected:
    std::function<void(PartialFD const&)> fd_consumer_;
    std::function<void(PartialKey const&)> ucc_consumer_;

    void DiscoverFd(PartialFD const& fd) {
        std::scoped_lock lock(discover_fd_mutex_);
        discovered_fds_.push_back(fd);
    }

    void DiscoverUcc(PartialKey const& key) {
        std::scoped_lock lock(discover_ucc_mutex_);
        discovered_uccs_.push_back(key);
    }

public:
    PartialFD RegisterFd(boost::dynamic_bitset<> const& lhs, model::Index rhs, double error,
                         double score) const;
    PartialKey RegisterUcc(boost::dynamic_bitset<> const& key_vertical, double error,
                           double score) const;

    virtual ~DependencyConsumer() = default;
};
