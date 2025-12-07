#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/util/dynamic_bitset.h"

namespace algos::dd {

/**
 * A trie-like structure (prefix tree) for storing bitsets and checking
 * if any stored bitset is a subset of a given bitset.
 */
class NTreeSearch {
private:
    // Maps a bit-position to a child node
    std::vector<std::unique_ptr<NTreeSearch>> children_;
    // Bitset that shows for each bit-position whether a child node is present in the map
    util::DynamicBitset<> children_bitset_;

    // Optional to hold a terminal bitset at this node.
    // If present, it represents a complete bitset stored here.
    std::optional<util::DynamicBitset<>> stored_bitset_;

    void InsertImpl(util::DynamicBitset<> const& bs, std::size_t cur_bit, std::size_t next_bit) {
        if (next_bit == util::DynamicBitset<>::npos) {
            stored_bitset_ = bs;
            return;
        }

        if (children_bitset_.none()) {
            if (!stored_bitset_) {
                stored_bitset_ = bs;
                return;
            } else {
                std::size_t stored_next_bit = cur_bit == util::DynamicBitset<>::npos
                                                      ? stored_bitset_->find_first()
                                                      : stored_bitset_->find_next(cur_bit);
                if (stored_next_bit != util::DynamicBitset<>::npos) {
                    if (children_.empty()) {
                        children_.resize(bs.size());
                    }
                    children_[stored_next_bit] =
                            std::make_unique<NTreeSearch>(stored_bitset_->size(), stored_bitset_);
                    children_bitset_.set(stored_next_bit, true);
                    stored_bitset_.reset();
                }
            }
        }

        if (children_.empty()) {
            children_.resize(bs.size());
        }
        auto& child = children_[next_bit];
        if (!child) {
            child = std::make_unique<NTreeSearch>(bs.size());
            children_bitset_.set(next_bit, true);
        }

        child->InsertImpl(bs, next_bit, bs.find_next(next_bit));
    }

    bool FindSubset(util::DynamicBitset<> const& bs, std::size_t next_bit) const {
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            return stored_bitset_->is_subset_of(bs);
        }

        while (next_bit != util::DynamicBitset<>::npos) {
            std::size_t next_index = bs.find_next(next_bit);
            if (children_bitset_[next_bit]) {
                if (children_[next_bit]->FindSubset(bs, next_index)) {
                    return true;
                }
            }
            next_bit = next_index;
        }

        return false;
    }

    bool GetAndRemoveGeneralizations(util::DynamicBitset<> const& bs, std::size_t next_bit,
                                     std::vector<util::DynamicBitset<>>& result) {
        if (stored_bitset_) {
            if (stored_bitset_->is_subset_of(bs)) {
                result.push_back(stored_bitset_.value());
                stored_bitset_.reset();
            } else {
                return false;
            }
        }

        while (next_bit != util::DynamicBitset<>::npos) {
            std::size_t next_index = bs.find_next(next_bit);
            if (children_bitset_[next_bit]) {
                if (children_[next_bit]->GetAndRemoveGeneralizations(bs, next_index, result)) {
                    children_[next_bit] = nullptr;
                    children_bitset_.set(next_bit, false);
                }
            }
            next_bit = next_index;
        }

        return children_bitset_.none();
    }

public:
    void Insert(boost::dynamic_bitset<> const& bs) {
        util::DynamicBitset bitset(bs);
        InsertImpl(bitset, util::DynamicBitset<>::npos, bitset.find_first());
    }

    [[nodiscard]]
    bool ContainsSubset(boost::dynamic_bitset<> const& bs) const {
        util::DynamicBitset bitset(bs);
        return FindSubset(bitset, bitset.find_first());
    }

    std::vector<util::DynamicBitset<>> GetAndRemoveGeneralizations(
            boost::dynamic_bitset<> const& bs) {
        std::vector<util::DynamicBitset<>> removed;
        util::DynamicBitset bitset(bs);
        GetAndRemoveGeneralizations(bitset, bitset.find_first(), removed);
        return removed;
    }

    void ForEach(std::function<void(boost::dynamic_bitset<> const&)> const& consumer) {
        if (stored_bitset_) {
            consumer(stored_bitset_->to_boost_dynamic_bitset());
        }

        for (std::size_t index = children_bitset_.find_first();
             index != util::DynamicBitset<>::npos; index = children_bitset_.find_next(index)) {
            children_[index]->ForEach(consumer);
        }
    }

    explicit NTreeSearch(std::size_t bitset_size = 64UL)
        : children_(), children_bitset_(bitset_size), stored_bitset_() {}

    NTreeSearch(std::size_t bitset_size, std::optional<util::DynamicBitset<>> const& bs)
        : children_(), children_bitset_(bitset_size), stored_bitset_(bs) {}
};

}  // namespace algos::dd
