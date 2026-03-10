#pragma once

#include <deque>
#include <memory>
#include <ranges>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/algorithms/fd/multi_attr_rhs_stripped_fd.h"
#include "core/config/max_lhs/type.h"
#include "core/model/table/table_header.h"
#include "core/util/bitset_utils.h"

namespace algos {
class MultiAttrRhsFdStorage {
public:
    // Use this, not the class itself, don't waste memory copying
    using OwningPointer = std::shared_ptr<MultiAttrRhsFdStorage>;

private:
    using Storage = std::deque<MultiAttrRhsStrippedFd>;

    class BuilderBase {
    protected:
        Storage stripped_fds_;

    public:
        OwningPointer Build(model::TableHeader table_header) {
            return std::make_shared<MultiAttrRhsFdStorage>(std::move(table_header),
                                                           std::move(stripped_fds_));
        }

        void Reset() {
            stripped_fds_.clear();
        }
    };

    model::TableHeader table_header_;
    // A typical LHS doesn't use up that much memory, and a processable dataset doesn't usually have
    // more than 256 columns. In lots of cases, we can store the numbers themselves compactly. But
    // that's for someone else.
    Storage stripped_fds_;

    auto FullView() const {
        return std::ranges::ref_view{stripped_fds_} |
               std::views::transform(
                       [table_header = &table_header_](MultiAttrRhsStrippedFd const& stripped_fd) {
                           return stripped_fd.ToFd(*table_header);
                       });
    }

public:
    // Use this to build the storage if your algorithm does not prune FDs with LHS size that is too
    // high on its own.
    class LhsLimBuilder : public BuilderBase {
        config::MaxLhsType max_lhs_;

    public:
        LhsLimBuilder(config::MaxLhsType max_lhs) : max_lhs_(max_lhs) {}

        void AddFd(MultiAttrRhsStrippedFd fd) {
            if (fd.lhs.count() > max_lhs_) return;
            stripped_fds_.push_back(std::move(fd));
        }
    };

    // Use this to build the storage if your algorithm prunes FDs with LHS size that is too high
    // on its own.
    class PlainBuilder : public BuilderBase {
    public:
        void AddFd(MultiAttrRhsStrippedFd fd) {
            stripped_fds_.push_back(std::move(fd));
        }
    };

    // Should not be used inside algorithms, use a builder.
    MultiAttrRhsFdStorage(model::TableHeader table_header, Storage stripped_fds)
        : table_header_(std::move(table_header)), stripped_fds_(std::move(stripped_fds)) {}

    Storage const& GetStripped() const noexcept {
        return stripped_fds_;
    }

    model::TableHeader const& GetTableHeader() const noexcept {
        return table_header_;
    }

    auto begin() const {
        return FullView().begin();
    }

    auto end() const {
        return FullView().end();
    }
};
}  // namespace algos
