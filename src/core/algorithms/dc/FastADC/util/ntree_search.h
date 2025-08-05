#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include <boost/dynamic_bitset.hpp>

namespace algos::fastadc {

/**
 * A trie-like structure (prefix tree) for storing bitsets and checking
 * if any stored bitset is a subset of a given bitset.
 */
class NTreeSearch {
public:
    void Insert(boost::dynamic_bitset<> const& bs) {
        InsertImpl(bs, bs.find_first());
    }

    [[nodiscard]]
    bool ContainsSubset(boost::dynamic_bitset<> const& bs) const {
        return FindSubset(bs, bs.find_first());
    }

private:
    // Maps a bit-position to a child node
    std::unordered_map<size_t, std::unique_ptr<NTreeSearch>> children_;

    // Optional to hold a terminal bitset at this node.
    // If present, it represents a complete bitset stored here.
    std::optional<boost::dynamic_bitset<>> stored_bitset_;

    void InsertImpl(boost::dynamic_bitset<> const& bs, size_t nextBit) {
        if (nextBit == boost::dynamic_bitset<>::npos) {
            stored_bitset_ = bs;
            return;
        }

        auto& child = children_[nextBit];
        if (!child) {
            child = std::make_unique<NTreeSearch>();
        }

        child->InsertImpl(bs, bs.find_next(nextBit));
    }

    bool FindSubset(boost::dynamic_bitset<> const& bs, size_t nextBit) const {
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            return true;
        }

        while (nextBit != boost::dynamic_bitset<>::npos) {
            if (auto it = children_.find(nextBit); it != children_.end()) {
                if (it->second->FindSubset(bs, bs.find_next(nextBit))) {
                    return true;
                }
            }
            nextBit = bs.find_next(nextBit);
        }

        return false;
    }
};

}  // namespace algos::fastadc
