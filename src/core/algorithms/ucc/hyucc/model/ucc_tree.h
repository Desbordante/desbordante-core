#pragma once

#include <memory>    // for unique_ptr
#include <stddef.h>  // for size_t
#include <vector>    // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset

#include "ucc_tree_vertex.h"  // for UCCTreeVertex

namespace algos::hyucc {

class UCCTree {
private:
    std::unique_ptr<UCCTreeVertex> root_;

public:
    explicit UCCTree(size_t num_attributes) : root_(UCCTreeVertex::Create(num_attributes, false)) {
        root_->InitChildren(true);
    }

    [[nodiscard]] size_t GetNumAttributes() const noexcept {
        return root_->GetNumAttributes();
    }

    [[nodiscard]] std::vector<boost::dynamic_bitset<>> GetUCCAndGeneralizations(
            boost::dynamic_bitset<> const& ucc) {
        return root_->GetUCCAndGeneralizationsRecursive(ucc);
    }

    void Remove(boost::dynamic_bitset<> const& ucc) {
        root_->RemoveRecursive(ucc, ucc.find_first());
    }

    [[nodiscard]] bool FindUCCOrGeneralization(boost::dynamic_bitset<> const& ucc) {
        return root_->FindUCCOrGeneralizationRecursive(ucc, ucc.find_first());
    }

    [[nodiscard]] std::vector<LhsPair> GetLevel(unsigned target_level) {
        return root_->GetLevelRecursive(target_level);
    }

    UCCTreeVertex* AddUCC(boost::dynamic_bitset<> const& ucc, bool* is_new_out = nullptr);
    [[nodiscard]] UCCTreeVertex* AddUCCGetIfNew(boost::dynamic_bitset<> const& ucc);
    [[nodiscard]] std::vector<boost::dynamic_bitset<>> FillUCCs() const;
};

}  // namespace algos::hyucc
