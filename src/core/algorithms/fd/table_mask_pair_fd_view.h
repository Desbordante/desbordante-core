#pragma once

#include <deque>
#include <memory>
#include <ranges>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/algorithms/fd/table_mask_pair.h"
#include "core/config/max_lhs/type.h"
#include "core/model/table/table_header.h"
#include "core/util/bitset_utils.h"

namespace algos::fd {
class TableMaskPairFdView {
public:
    // Use this, not the class itself, don't waste memory copying
    using OwningPointer = std::shared_ptr<TableMaskPairFdView>;
    using Storage = std::deque<TableMaskPair>;

private:
    model::TableHeader table_header_;
    // A typical LHS doesn't use up that much memory, and a processable dataset doesn't usually have
    // more than 256 columns. In lots of cases, we can store the numbers themselves compactly.
    // Perhaps there is a use case for compressed storage in memory.
    Storage table_mask_pairs_;

    auto FdView() const {
        return std::ranges::ref_view{table_mask_pairs_} |
               std::views::transform(
                       [table_header = &table_header_](TableMaskPair const& stripped_fd) {
                           return stripped_fd.ToFd(*table_header);
                       });
    }

public:
    TableMaskPairFdView(model::TableHeader table_header, Storage stripped_fds)
        : table_header_(std::move(table_header)), table_mask_pairs_(std::move(stripped_fds)) {}

    Storage const& GetTableMaskPairs() const noexcept {
        return table_mask_pairs_;
    }

    model::TableHeader const& GetTableHeader() const noexcept {
        return table_header_;
    }

    auto begin() const {
        return FdView().begin();
    }

    auto end() const {
        return FdView().end();
    }
};
}  // namespace algos::fd
