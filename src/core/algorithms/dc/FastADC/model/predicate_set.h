#pragma once

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <assert.h>
#include <boost/move/utility_core.hpp>
#include <iterator>
#include <memory>
#include <cstddef>
#include <string>
#include <variant>

#include "dc/FastADC/providers/index_provider.h"
#include "predicate.h"

namespace algos::fastadc {

class PredicateSet {
private:
    boost::dynamic_bitset<> bitset_{kPredicateBits};
    mutable std::unique_ptr<PredicateSet> inv_set_TS_;  // Cached inverse set

public:
    // We want to iterate over PredicateSet with range-based for.
    // In order to do this, PredicateSet should contain valid predicate
    // index provider, hence deleting the default constructor
    PredicateSet() = delete;

    PredicateIndexProvider* provider = nullptr;

    explicit PredicateSet(PredicateIndexProvider* predicate_index_provider)
        : bitset_(0), provider(predicate_index_provider) {
        assert(predicate_index_provider);
    }

    explicit PredicateSet(boost::dynamic_bitset<> const& bitset,
                          PredicateIndexProvider* predicate_index_provider)
        : bitset_(bitset), provider(predicate_index_provider) {
        assert(predicate_index_provider);
    }

    PredicateSet(PredicateSet const& other) : bitset_(other.bitset_), provider(other.provider) {
        // Do not copy inv_set_TS_ to avoid unnecessary pre-caching
    }

    PredicateSet& operator=(PredicateSet const& other) {
        if (this != &other) {
            bitset_ = other.bitset_;
            provider = other.provider;
            inv_set_TS_.reset();
        }
        return *this;
    }

    PredicateSet(PredicateSet&& other) noexcept = default;
    PredicateSet& operator=(PredicateSet&& other) noexcept = default;

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
    PredicateSet GetInvTS(PredicateProvider* predicate_provider) const;

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
            return set_->provider->GetObject(current_index_);
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

}  // namespace algos::fastadc

template <>
struct std::hash<algos::fastadc::PredicateSet> {
    size_t operator()(algos::fastadc::PredicateSet const& k) const noexcept {
        return k.Hash();
    }
};
