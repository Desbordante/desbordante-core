#include "fd_tree_vertex.h"

#include <algorithm>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace model {

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

bool FDTreeVertex::ContainsFdOrGeneralRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                                size_t cur_bit) const {
    if (IsFd(rhs)) {
        return true;
    }

    if (!HasChildren()) {
        return false;
    }

    for (; cur_bit != boost::dynamic_bitset<>::npos; cur_bit = lhs.find_next(cur_bit)) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs) &&
            children_[cur_bit]->ContainsFdOrGeneralRecursive(lhs, rhs, lhs.find_next(cur_bit))) {
            return true;
        }
    }

    return false;
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

        if (children_[current_lhs_attr]->GetAttributes().none()) {
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

void FDTreeVertex::RemoveSpecialsRecursive(boost::dynamic_bitset<> const& lhs,
                                           size_t rhs, size_t cur_bit,
                                           bool is_specialized) {
    // TODO: optimize checking via counting bits
    size_t next_lhs_bit;
    if (cur_bit < lhs.size()) {
        next_lhs_bit = lhs.test(cur_bit) ? cur_bit : lhs.find_next(cur_bit);
    } else {
        next_lhs_bit = boost::dynamic_bitset<>::npos;
    }

    if (is_specialized && next_lhs_bit == boost::dynamic_bitset<>::npos && IsFd(rhs)) {
        RemoveFd(rhs);
        RemoveAttribute(rhs);
        return;
    }

    if (HasChildren()) {
        auto limit = next_lhs_bit == boost::dynamic_bitset<>::npos
                ? num_attributes_ - 1
                : next_lhs_bit;

        for (; cur_bit <= limit; ++cur_bit) {
            if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
                children_[cur_bit]->RemoveSpecialsRecursive(lhs, rhs, cur_bit + 1, 
                                                            is_specialized || cur_bit != next_lhs_bit);

                if (children_[cur_bit]->GetAttributes().none()) {
                    children_[cur_bit] = nullptr;
                    children_count_--;
                }
            }
        }
    }

    if (!IsFd(rhs) && IsLastNodeOf(rhs)) {
        RemoveAttribute(rhs);
    }
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

}  // namespace model
