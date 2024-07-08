#pragma once

#include <cstddef>
#include <iterator>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/md_element.h"
#include "algorithms/md/hymd/rhss.h"
#include "model/index.h"

namespace algos::hymd::utility {
class InvalidatedRhss {
    using NewCCVIds = std::vector<ColumnClassifierValueId>;

    Rhss invalidated_;
    NewCCVIds new_ccv_ids_;

public:
    class UpdateView {
        InvalidatedRhss const& invalidated_info_;

        UpdateView(InvalidatedRhss const& invalidated_info) noexcept
            : invalidated_info_(invalidated_info) {}

    public:
        class update_iterator {
            using NewCCVIdsIterator = NewCCVIds::const_iterator;
            using InvalidatedIterator = Rhss::const_iterator;

            InvalidatedIterator invalidated_iter_;
            NewCCVIdsIterator new_ccv_ids_iter_;

            update_iterator(InvalidatedIterator invalidated_iter,
                            NewCCVIdsIterator new_ccv_ids_iter) noexcept
                : invalidated_iter_(invalidated_iter), new_ccv_ids_iter_(new_ccv_ids_iter) {}

        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = MdElement;
            using pointer = void;
            using reference = void;

            update_iterator operator++() noexcept {
                ++invalidated_iter_;
                ++new_ccv_ids_iter_;
                return *this;
            }

            update_iterator operator++(int) noexcept {
                update_iterator old = *this;
                ++(*this);
                return old;
            }

            friend bool operator==(update_iterator const& iter1, update_iterator const& iter2) {
                return iter1.invalidated_iter_ == iter2.invalidated_iter_;
            }

            friend bool operator!=(update_iterator const& iter1, update_iterator const& iter2) {
                return !(iter1 == iter2);
            }

            value_type operator*() const noexcept {
                return {invalidated_iter_->index, *new_ccv_ids_iter_};
            }

            friend UpdateView;
        };

        update_iterator begin() const noexcept {
            return {invalidated_info_.invalidated_.begin(), invalidated_info_.new_ccv_ids_.begin()};
        }

        update_iterator end() const noexcept {
            return {invalidated_info_.invalidated_.end(), invalidated_info_.new_ccv_ids_.end()};
        }

        friend InvalidatedRhss;
    };

    void PushBack(MdElement old_rhs, ColumnClassifierValueId new_ccv_id) {
        invalidated_.push_back(old_rhs);
        new_ccv_ids_.push_back(new_ccv_id);
    }

    UpdateView GetUpdateView() const noexcept {
        return {*this};
    }

    Rhss const& GetInvalidated() const noexcept {
        return invalidated_;
    }

    bool IsEmpty() const noexcept {
        return invalidated_.empty();
    }

    std::size_t Size() const noexcept {
        return invalidated_.size();
    }
};
}  // namespace algos::hymd::utility
