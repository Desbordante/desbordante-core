#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/rhss.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::cover_calculation {
class InvalidatedRhss {
    using NewRCVIds = std::vector<RecordClassifierValueId>;

    lattice::Rhss invalidated_;
    NewRCVIds new_rcv_ids_;

public:
    class UpdateView {
        using InvalidatedIterator = lattice::Rhss::const_iterator;

        InvalidatedRhss const& invalidated_info_;

        UpdateView(InvalidatedRhss const& invalidated_info) noexcept
            : invalidated_info_(invalidated_info) {}

    public:
        class UpdateIterator {
            using NewRCVIdsIterator = NewRCVIds::const_iterator;

            InvalidatedIterator invalidated_iter_;
            NewRCVIdsIterator new_rcv_ids_iter_;

            UpdateIterator(InvalidatedIterator invalidated_iter,
                           NewRCVIdsIterator new_rcv_ids_iter) noexcept
                : invalidated_iter_(invalidated_iter), new_rcv_ids_iter_(new_rcv_ids_iter) {}

        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = MdeElement;
            using pointer = void;
            using reference = void;

            UpdateIterator operator++() noexcept {
                ++invalidated_iter_;
                ++new_rcv_ids_iter_;
                return *this;
            }

            UpdateIterator operator++(int) noexcept {
                UpdateIterator old = *this;
                ++(*this);
                return old;
            }

            friend bool operator==(UpdateIterator const& iter1, InvalidatedIterator const& iter2) {
                return iter1.invalidated_iter_ == iter2;
            }

            friend bool operator!=(UpdateIterator const& iter1, InvalidatedIterator const& iter2) {
                return !(iter1 == iter2);
            }

            value_type operator*() const noexcept {
                return {invalidated_iter_->record_match_index, *new_rcv_ids_iter_};
            }

            friend UpdateView;
        };

        UpdateIterator begin() const noexcept {
            return {invalidated_info_.invalidated_.begin(), invalidated_info_.new_rcv_ids_.begin()};
        }

        InvalidatedIterator end() const noexcept {
            return invalidated_info_.invalidated_.end();
        }

        friend InvalidatedRhss;
    };

    void PushBack(MdeElement old_rhs, RecordClassifierValueId new_rcv_id) {
        invalidated_.push_back(old_rhs);
        new_rcv_ids_.push_back(new_rcv_id);
    }

    UpdateView GetUpdateView() const noexcept {
        return {*this};
    }

    lattice::Rhss const& GetInvalidated() const noexcept {
        return invalidated_;
    }

    bool IsEmpty() const noexcept {
        return invalidated_.empty();
    }

    std::size_t Size() const noexcept {
        return invalidated_.size();
    }
};
}  // namespace algos::hymde::cover_calculation
