#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_pair_fd.h"
#include "core/config/max_lhs/type.h"

namespace algos {
class BitsetPairFdStorage {
public:
    using BackingStorage = std::deque<BitsetPairFd>;

private:
    class BuilderBase {
    protected:
        BackingStorage storage_;

    public:
        void Reset() {
            storage_.clear();
        }
    };

protected:
    // Maybe it would be a good idea to make it possible to avoid storing the result at all and just
    // pass a function here, but over-abstraction for the sake of maybes has not historically
    // produced great results in this project (see RelationalSchema, ColumnLayoutRelationalData).
    BackingStorage fd_view_;

public:
    // Use this to build the storage if your algorithm does not prune FDs with LHS size that is too
    // high on its own.
    class LhsLimBuilder : public BuilderBase {
        config::MaxLhsType max_lhs_;

    public:
        LhsLimBuilder(config::MaxLhsType max_lhs) : max_lhs_(max_lhs) {}

        void AddFd(BitsetPairFd fd) {
            if (fd.lhs.count() > max_lhs_) return;
            storage_.push_back(std::move(fd));
        }
    };

    // Use this to build the storage if your algorithm prunes FDs with LHS size that is too high
    // on its own.
    class PlainBuilder : public BuilderBase {
    public:
        void AddFd(BitsetPairFd fd) {
            storage_.push_back(std::move(fd));
        }
    };
};
}  // namespace algos
