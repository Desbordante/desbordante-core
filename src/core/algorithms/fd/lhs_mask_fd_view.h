#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <ranges>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/algorithms/fd/lhs_table_mask.h"
#include "core/config/max_lhs/type.h"
#include "core/model/index.h"
#include "core/model/table/table_header.h"
#include "core/util/bitset_utils.h"

namespace algos::fd {
class LhsMaskFdView {
public:
    // Use this, not the class itself, don't waste memory copying
    using OwningPointer = std::shared_ptr<LhsMaskFdView>;
    using Storage = std::vector<std::deque<LhsTableMask>>;

private:
    class Iterator;

    class Sentinel {
        std::size_t end_id_;
        friend class Iterator;

    public:
        explicit Sentinel(std::size_t end_id) noexcept : end_id_(end_id) {}
    };

    // Are you looking at this class trying to copy? See TableMaskPairFdView first, a simpler
    // approach from there would be better, if possible.
    class Iterator {
        LhsMaskFdView const* storage_;
        std::size_t rhs_attr_id_;
        Storage::value_type::const_iterator rhs_attr_fds_it_;
        Storage::value_type::const_iterator rhs_attr_fds_end_;

        void AdvanceToValid() noexcept {
            auto const& fds = storage_->lhs_masks_;
            while (rhs_attr_id_ != fds.size() && rhs_attr_fds_it_ == rhs_attr_fds_end_) {
                ++rhs_attr_id_;
                if (rhs_attr_id_ != fds.size()) {
                    rhs_attr_fds_it_ = fds[rhs_attr_id_].begin();
                    rhs_attr_fds_end_ = fds[rhs_attr_id_].end();
                }
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = model::FunctionalDependency;
        using difference_type = std::ptrdiff_t;
        using reference = model::FunctionalDependency;

        Iterator(LhsMaskFdView const* storage, std::size_t rhs_attr_id,
                 Storage::value_type::const_iterator it,
                 Storage::value_type::const_iterator end) noexcept
            : storage_(storage),
              rhs_attr_id_(rhs_attr_id),
              rhs_attr_fds_it_(it),
              rhs_attr_fds_end_(end) {
            AdvanceToValid();
        }

        model::FunctionalDependency operator*() const {
            return rhs_attr_fds_it_->ToFd(storage_->table_header_, rhs_attr_id_);
        }

        Iterator& operator++() noexcept {
            ++rhs_attr_fds_it_;
            AdvanceToValid();
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(Sentinel const& s) const noexcept {
            return rhs_attr_id_ == s.end_id_;
        }
    };

    model::TableHeader table_header_;
    // A typical LHS doesn't use up that much memory, and a processable dataset doesn't usually have
    // more than 256 columns. In lots of cases, we can store the numbers themselves compactly.
    // Perhaps there is a use case for compressed storage in memory.
    Storage lhs_masks_;

    // C++23
#if 0
    auto FullView() const {
        return std::ranges::ref_view{lhs_masks_} | std::views::enumerate |
               std::views::transform([table_header = &table_header_](auto const& pair) {
                   auto const& [rhs_attr_index, attr_fds] = pair;
                   return std::ranges::ref_view{attr_fds} |
                          std::views::transform(
                                  [rhs_attr_index,
                                   table_header](LhsTableMask const& stripped_fd) {
                                      return stripped_fd.ToFd(*table_header, rhs_attr_index);
                                  });
               }) |
               std::views::join;
    }
#endif

public:
    LhsMaskFdView(model::TableHeader table_header, Storage stripped_fds)
        : table_header_(std::move(table_header)), lhs_masks_(std::move(stripped_fds)) {}

    Storage const& GetLhsMasks() const noexcept {
        return lhs_masks_;
    }

    model::TableHeader const& GetTableHeader() const noexcept {
        return table_header_;
    }

    auto begin() const {
        if (lhs_masks_.empty()) return Iterator{this, 0, {}, {}};
        return Iterator{this, 0, lhs_masks_[0].begin(), lhs_masks_[0].end()};
        // return FullView().begin();
    }

    auto end() const {
        return Sentinel{lhs_masks_.size()};
        // return FullView().end();
    }
};
}  // namespace algos::fd
