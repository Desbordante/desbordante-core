#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <ranges>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/algorithms/fd/single_attr_rhs_stripped_fd.h"
#include "core/config/max_lhs/type.h"
#include "core/model/index.h"
#include "core/model/table/table_header.h"
#include "core/util/bitset_utils.h"

namespace algos {
class SingleAttrRhsFdStorage {
public:
    // Use this, not the class itself, don't waste memory copying
    using OwningPointer = std::shared_ptr<SingleAttrRhsFdStorage>;

private:
    using Storage = std::vector<std::deque<SingleAttrRhsStrippedFd>>;

    class BuilderBase {
    protected:
        Storage stripped_fds_;

    public:
        BuilderBase(std::size_t attr_num) : stripped_fds_(attr_num) {}

        OwningPointer Build(model::TableHeader table_header) {
            return std::make_shared<SingleAttrRhsFdStorage>(std::move(table_header),
                                                            std::move(stripped_fds_));
        }

        void Reset() {
            stripped_fds_.clear();
        }
    };

    class Iterator;

    class Sentinel {
        std::size_t end_id_;
        friend class Iterator;

    public:
        explicit Sentinel(std::size_t end_id) noexcept : end_id_(end_id) {}
    };

    // Are you looking at this class trying to copy? See MultiAttrRhsFdStorage first, a simpler
    // approach from there would be better, if possible.
    class Iterator {
        SingleAttrRhsFdStorage const* storage_;
        std::size_t rhs_attr_id_;
        Storage::value_type::const_iterator rhs_attr_fds_it_;
        Storage::value_type::const_iterator rhs_attr_fds_end_;

        void AdvanceToValid() noexcept {
            auto const& fds = storage_->stripped_fds_;
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

        Iterator(SingleAttrRhsFdStorage const* storage, std::size_t rhs_attr_id,
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
    // more than 256 columns. In lots of cases, we can store the numbers themselves compactly. But
    // that's for someone else.
    Storage stripped_fds_;

    // C++23
#if 0
    auto FullView() const {
        return std::ranges::ref_view{stripped_fds_} | std::views::enumerate |
               std::views::transform([table_header = &table_header_](auto const& pair) {
                   auto const& [rhs_attr_index, attr_fds] = pair;
                   return std::ranges::ref_view{attr_fds} |
                          std::views::transform(
                                  [rhs_attr_index,
                                   table_header](SingleAttrRhsStrippedFd const& stripped_fd) {
                                      return stripped_fd.ToFd(*table_header, rhs_attr_index);
                                  });
               }) |
               std::views::join;
    }
#endif

public:
    // Use this to build the storage if your algorithm does not prune FDs with LHS size that is too
    // high on its own.
    class LhsLimBuilder : public BuilderBase {
        config::MaxLhsType max_lhs_;

    public:
        LhsLimBuilder(std::size_t attr_num, config::MaxLhsType max_lhs)
            : BuilderBase(attr_num), max_lhs_(max_lhs) {}

        void AddFd(model::Index attr_index, SingleAttrRhsStrippedFd fd) {
            if (fd.lhs.count() > max_lhs_) return;
            stripped_fds_[attr_index].push_back(std::move(fd));
        }
    };

    // Use this to build the storage if your algorithm prunes FDs with LHS size that is too high
    // on its own.
    class PlainBuilder : public BuilderBase {
    public:
        void AddFd(model::Index attr_index, SingleAttrRhsStrippedFd fd) {
            stripped_fds_[attr_index].push_back(std::move(fd));
        }
    };

    // Should not be used inside algorithms, use a builder.
    SingleAttrRhsFdStorage(model::TableHeader table_header, Storage stripped_fds)
        : table_header_(std::move(table_header)), stripped_fds_(std::move(stripped_fds)) {}

    Storage const& GetStripped() const noexcept {
        return stripped_fds_;
    }

    model::TableHeader const& GetTableHeader() const noexcept {
        return table_header_;
    }

    auto begin() const {
        if (stripped_fds_.empty()) return Iterator{this, 0, {}, {}};
        return Iterator{this, 0, stripped_fds_[0].begin(), stripped_fds_[0].end()};
        // return FullView().begin();
    }

    auto end() const {
        // return FullView().end();
        return Sentinel{stripped_fds_.size()};
    }
};
}  // namespace algos
