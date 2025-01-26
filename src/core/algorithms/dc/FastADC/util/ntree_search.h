#pragma once

#include <memory>
#include <unordered_map>

#include <boost/dynamic_bitset.hpp>

namespace algos::fastadc {
class NTreeSearch {
public:
    void Add(boost::dynamic_bitset<> const& bs) {
        Add(bs, bs.find_first());
    }

    bool ContainsSubset(boost::dynamic_bitset<> const& add) const {
        return GetSubset(add, add.find_first()) != nullptr;
    }

private:
    std::unordered_map<size_t, std::unique_ptr<NTreeSearch>> subtrees_;
    std::optional<boost::dynamic_bitset<>> bitset_;

    void Add(boost::dynamic_bitset<> const& bs, size_t next_bit) {
        if (next_bit == boost::dynamic_bitset<>::npos) {
            bitset_ = bs;  // Store bitset at the current node
            return;
        }

        // Create a new subtree at the position of the next set bit, if needed
        auto& subtree = subtrees_[next_bit];
        if (!subtree) {
            subtree = std::make_unique<NTreeSearch>();
        }

        // Add the bitset to the next subtree
        subtree->Add(bs, bs.find_next(next_bit));
    }

    boost::dynamic_bitset<> const* GetSubset(boost::dynamic_bitset<> const& add,
                                             size_t next_bit) const {
        if (bitset_) {
            return &bitset_.value();
        }

        while (next_bit != boost::dynamic_bitset<>::npos) {
            auto it = subtrees_.find(next_bit);
            if (it != subtrees_.end()) {
                boost::dynamic_bitset<> const* res =
                        it->second->GetSubset(add, add.find_next(next_bit));
                if (res != nullptr) {
                    return res;
                }
            }
            next_bit = add.find_next(next_bit);
        }

        return nullptr;
    }
};
}  // namespace algos::fastadc
