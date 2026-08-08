
#include "core/algorithms/fd/pyrocommon/core/dependency_consumer.h"

PartialFD DependencyConsumer::RegisterFd(boost::dynamic_bitset<> const& lhs, model::Index rhs,
                                         double error, double score) const {
    PartialFD partial_fd(lhs, rhs, error, score);
    fd_consumer_(partial_fd);
    return partial_fd;
}

PartialKey DependencyConsumer::RegisterUcc(boost::dynamic_bitset<> const& key_vertical,
                                           double error, double score) const {
    PartialKey partial_key(key_vertical, error, score);
    ucc_consumer_(partial_key);
    return partial_key;
}
