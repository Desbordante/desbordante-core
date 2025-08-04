#include "fd_tree_vertex.h"

#include <algorithm>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::hyfd::fd_tree {

void FDTreeVertex::GetLevelRecursive(unsigned target_level, unsigned cur_level,
                                     boost::dynamic_bitset<> lhs, std::vector<LhsPair>& vertices) {
    if (cur_level == target_level) {
        if (fds_.any()) {
            vertices.emplace_back(shared_from_this(), lhs);
        }
        return;
    }

    if (!HasChildren()) {
        return;
    }

    for (size_t i = cur_level; i < num_attributes_; ++i) {
        if (ContainsChildAt(i)) {
            lhs.set(i);

            children_[i]->GetLevelRecursive(target_level, cur_level + 1, lhs, vertices);

            lhs.reset(i);
        }
    }
}

void FDTreeVertex::GetFdAndGeneralsRecursive(boost::dynamic_bitset<> const& lhs,
                                             boost::dynamic_bitset<> cur_lhs, size_t rhs,
                                             size_t cur_bit,
                                             std::vector<boost::dynamic_bitset<>>& result) const {
    if (IsFd(rhs)) {
        result.push_back(cur_lhs);
        return;  // If this vertex has the RHS bit set, then none of its children will have
                 // this bit set.
    }

    if (!HasChildren()) {
        return;
    }

    for (; cur_bit != boost::dynamic_bitset<>::npos; cur_bit = lhs.find_next(cur_bit)) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->GetFdAndGeneralsRecursive(lhs, cur_lhs, rhs, lhs.find_next(cur_bit),
                                                          result);
            cur_lhs.reset(cur_bit);
        }
    }
}

bool FDTreeVertex::FindFdOrGeneralRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                            size_t cur_bit) const {
    if (IsFd(rhs)) {
        return true;
    }

    if (cur_bit == boost::dynamic_bitset<>::npos) {
        return false;
    }

    if (HasChildren() && ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs) &&
        children_[cur_bit]->FindFdOrGeneralRecursive(lhs, rhs, lhs.find_next(cur_bit))) {
        return true;
    }

    return FindFdOrGeneralRecursive(lhs, rhs, lhs.find_next(cur_bit));
}

bool FDTreeVertex::RemoveRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                   size_t current_lhs_attr) {
    if (current_lhs_attr == boost::dynamic_bitset<>::npos) {
        RemoveFd(rhs);
        RemoveAttribute(rhs);
        return true;
    }

    if (HasChildren() && ContainsChildAt(current_lhs_attr)) {
        if (!children_[current_lhs_attr]->RemoveRecursive(lhs, rhs,
                                                          lhs.find_next(current_lhs_attr))) {
            return false;
        }

        if (!children_[current_lhs_attr]->GetAttributes().any()) {
            children_[current_lhs_attr] = nullptr;
            children_count_--;
        }
    }

    if (IsLastNodeOf(rhs)) {
        RemoveAttribute(rhs);
        return true;
    }
    return false;
}

bool FDTreeVertex::IsLastNodeOf(size_t rhs) const noexcept {
    if (!HasChildren()) {
        return true;
    }
    return std::none_of(children_.cbegin(), children_.cend(), [rhs](auto const& child) {
        return (child != nullptr) && child->IsAttribute(rhs);
    });
}

std::shared_ptr<FDTreeVertex> FDTreeVertex::GetChildIfExists(size_t pos) const {
    if (children_.empty()) {
        return nullptr;
    }

    assert(pos < children_.size());
    return children_[pos];
}

void FDTreeVertex::FillFDs(std::vector<RawFD>& fds, boost::dynamic_bitset<>& lhs) const {
    for (size_t rhs = fds_.find_first(); rhs != boost::dynamic_bitset<>::npos;
         rhs = fds_.find_next(rhs)) {
        fds.emplace_back(lhs, rhs);
    }

    if (!HasChildren()) {
        return;
    }

    for (size_t i = 0; i < GetNumAttributes(); i++) {
        if (!ContainsChildAt(i)) {
            continue;
        }

        lhs.set(i);
        GetChild(i)->FillFDs(fds, lhs);
        lhs.reset(i);
    }
}

}  // namespace algos::hyfd::fd_tree
