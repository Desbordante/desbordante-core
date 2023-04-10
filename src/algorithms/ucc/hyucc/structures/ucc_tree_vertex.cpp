#include "ucc_tree_vertex.h"

namespace algos {

void UCCTreeVertex::InitChildren(bool is_ucc) {
    assert(children_.empty());
    for (size_t i = 0; i != num_attributes_; ++i) {
        AddChild(i, is_ucc);
    }
}

bool UCCTreeVertex::AddChild(size_t pos, bool is_ucc) {
    if (children_.empty()) {
        children_.resize(num_attributes_);
    }

    if (!ContainsChildAt(pos)) {
        children_[pos] = Create(num_attributes_, is_ucc);
        return true;
    }

    return false;
}

void UCCTreeVertex::GetUCCAndGeneralizationsRecursiveImpl(
        boost::dynamic_bitset<> const& ucc, size_t cur_bit, boost::dynamic_bitset<> cur_ucc,
        std::vector<boost::dynamic_bitset<>>& res) {
    if (is_ucc_) {
        res.push_back(cur_ucc);
    }

    if (children_.empty()) {
        return;
    }

    while (cur_bit != boost::dynamic_bitset<>::npos) {
        size_t next_bit = ucc.find_next(cur_bit);

        if (auto const& child = children_[cur_bit]; child != nullptr) {
            cur_ucc.set(cur_bit);
            child->GetUCCAndGeneralizationsRecursiveImpl(ucc, next_bit, cur_ucc, res);
            cur_ucc.reset(cur_bit);
        }

        cur_bit = next_bit;
    }
}

[[nodiscard]] std::vector<boost::dynamic_bitset<>> UCCTreeVertex::GetUCCAndGeneralizationsRecursive(
        boost::dynamic_bitset<> const& ucc) {
    std::vector<boost::dynamic_bitset<>> ucc_and_generalizations;
    boost::dynamic_bitset<> cur_ucc(ucc.size());
    GetUCCAndGeneralizationsRecursiveImpl(ucc, ucc.find_first(), cur_ucc, ucc_and_generalizations);
    return ucc_and_generalizations;
}

void UCCTreeVertex::RemoveRecursive(boost::dynamic_bitset<> const& ucc, size_t cur_bit) {
    if (cur_bit == boost::dynamic_bitset<>::npos) {
        is_ucc_ = false;
        return;
    }

    if (UCCTreeVertex* child = GetChildIfExists(cur_bit); child != nullptr) {
        child->RemoveRecursive(ucc, ucc.find_next(cur_bit));

        if (child->IsObsolete()) {
            children_[cur_bit] = nullptr;
        }
    }
}

[[nodiscard]] bool UCCTreeVertex::FindUCCOrGeneralizationRecursive(
        boost::dynamic_bitset<> const& ucc, size_t cur_bit) const {
    if (is_ucc_) {
        return true;
    }

    if (cur_bit == boost::dynamic_bitset<>::npos) {
        return false;
    }

    size_t const next_bit = ucc.find_next(cur_bit);
    if (UCCTreeVertex* child = GetChildIfExists(cur_bit);
        child && child->FindUCCOrGeneralizationRecursive(ucc, next_bit)) {
        return true;
    }

    return FindUCCOrGeneralizationRecursive(ucc, next_bit);
}

void UCCTreeVertex::GetLevelRecursiveImpl(unsigned target_level, unsigned cur_level,
                                          boost::dynamic_bitset<> ucc,
                                          std::vector<LhsPair>& result) {
    if (target_level == cur_level) {
        result.emplace_back(this, ucc);
        return;
    }

    for (size_t i = 0; i != children_.size(); ++i) {
        if (children_[i] == nullptr) {
            continue;
        }

        ucc.set(i);
        children_[i]->GetLevelRecursiveImpl(target_level, cur_level + 1, ucc, result);
        ucc.reset(i);
    }
}

void UCCTreeVertex::FillUCCsRecursive(std::vector<boost::dynamic_bitset<>>& uccs,
                                      boost::dynamic_bitset<> ucc) {
    if (is_ucc_) {
        uccs.push_back(ucc);
    }

    for (size_t i = 0; i != children_.size(); ++i) {
        auto* child = GetChildIfExists(i);
        if (child == nullptr) {
            continue;
        }

        ucc.set(i);
        child->FillUCCsRecursive(uccs, ucc);
        ucc.reset(i);
    }
}

[[nodiscard]] std::vector<LhsPair> UCCTreeVertex::GetLevelRecursive(unsigned target_level) {
    std::vector<LhsPair> level;
    boost::dynamic_bitset<> ucc(num_attributes_);
    GetLevelRecursiveImpl(target_level, 0, std::move(ucc), level);
    return level;
}

}  // namespace algos
