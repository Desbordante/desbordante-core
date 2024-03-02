#pragma once

#include <iterator>
#include <memory>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "index_provider.h"
#include "predicate.h"

namespace model {

class PredicateSet {
private:
    boost::dynamic_bitset<> bitset_{64};
    mutable std::unique_ptr<PredicateSet> inv_set_TS_;  // Cached inverse set

public:
    PredicateSet() = default;

    explicit PredicateSet(boost::dynamic_bitset<> const& bitset) : bitset_(bitset) {}

    PredicateSet(PredicateSet const& other) : bitset_(other.bitset_) {
        // Do not copy inv_set_TS_ to avoid unnecessary pre-caching
    }

    PredicateSet& operator=(PredicateSet const& other) {
        if (this != &other) {
            bitset_ = other.bitset_;
            inv_set_TS_.reset();
        }
        return *this;
    }

    PredicateSet(PredicateSet&& other) noexcept
        : bitset_(std::move(other.bitset_)), inv_set_TS_(std::move(other.inv_set_TS_)) {}

    PredicateSet& operator=(PredicateSet&& other) noexcept {
        if (this != &other) {
            bitset_ = std::move(other.bitset_);
            inv_set_TS_ = std::move(other.inv_set_TS_);
        }
        return *this;
    }

    // Adds a predicate to the set. Returns true if the predicate was newly added.
    bool Add(PredicatePtr predicate);

    bool Contains(PredicatePtr predicate) const;

    boost::dynamic_bitset<> const& GetBitset() const {
        return bitset_;
    }

    size_t Size() const {
        return bitset_.count();
    }

    // Retrieves the inverse TS predicate set (with caching).
    PredicateSet GetInvTS() const;

    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = PredicatePtr;
        using difference_type = std::ptrdiff_t;
        using pointer = PredicatePtr;
        using reference = PredicatePtr;

    private:
        PredicateSet const* set_;
        size_t current_index_;

    public:
        Iterator(PredicateSet const* set, size_t start) : set_(set), current_index_(start) {
            if (current_index_ == 0) {
                current_index_ = set_->bitset_.find_first();
            } else if (current_index_ >= set_->bitset_.size()) {
                current_index_ = boost::dynamic_bitset<>::npos;
            }
        }

        reference operator*() const {
            return PredicateIndexProvider::GetInstance()->GetObject(current_index_);
        }

        pointer operator->() const {
            return operator*();
        }

        Iterator& operator++() {
            if (current_index_ != boost::dynamic_bitset<>::npos) {
                current_index_ = set_->bitset_.find_next(current_index_);
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.current_index_ == b.current_index_ && a.set_ == b.set_;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) {
            return !(a == b);
        }
    };

    // NOLINTBEGIN(readability-identifier-naming)
    Iterator begin() const;
    Iterator end() const;

    // NOLINTEND(readability-identifier-naming)

    size_t Hash() const {
        return std::hash<boost::dynamic_bitset<>>()(bitset_);
    }

    bool operator==(PredicateSet const& other) const {
        return bitset_ == other.bitset_;
    }

    bool operator!=(PredicateSet const& other) const {
        return !(*this == other);
    }

    std::string ToString() const;
};

}  // namespace model

namespace std {
template <>
struct hash<model::PredicateSet> {
    size_t operator()(model::PredicateSet const& k) const noexcept {
        return k.Hash();
    }
};
}  // namespace std
