#include "fd_tree_vertex.h"

#include <algorithm>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace model {

void FDTreeVertex::GetLevelRecursive(unsigned target_level, unsigned cur_level,
                                     boost::dynamic_bitset<> lhs, std::vector<LhsPair>& vertices) {
    if (cur_level == target_level) {
        vertices.emplace_back(shared_from_this(), lhs);
        return;
    }

    if (!HasChildren()) {
        return;
    }

    for (size_t i = 0; i < num_attributes_; ++i) {
        if (ContainsChildAt(i)) {
            lhs.set(i);

            children_[i]->GetLevelRecursive(target_level, cur_level + 1, lhs, vertices);

            lhs.reset(i);
        }
    }
}

void FDTreeVertex::GetGeneralsRecursive(boost::dynamic_bitset<> const& lhs,
                                        boost::dynamic_bitset<>& cur_lhs, size_t rhs,
                                        size_t cur_bit,
                                        std::vector<boost::dynamic_bitset<>>& result) const {
    // TODO: optimize checking via counting bits
    if (IsFd(rhs) && lhs != cur_lhs) {
        result.push_back(cur_lhs);
    }

    if (!HasChildren()) {
        return;
    }

    for (; cur_bit != boost::dynamic_bitset<>::npos; cur_bit = lhs.find_next(cur_bit)) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->GetGeneralsRecursive(lhs, cur_lhs, rhs, lhs.find_next(cur_bit),
                                                     result);
            cur_lhs.reset(cur_bit);
        }
    }
}

void FDTreeVertex::GetFdAndGeneralsRecursive(boost::dynamic_bitset<> const& lhs,
                                             boost::dynamic_bitset<> cur_lhs, size_t rhs,
                                             size_t cur_bit,
                                             std::vector<boost::dynamic_bitset<>>& result) const {
    if (IsFd(rhs)) {
        result.push_back(cur_lhs);
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

void FDTreeVertex::GetSpecialsRecursive(boost::dynamic_bitset<> const& lhs,
                                        boost::dynamic_bitset<>& cur_lhs, size_t rhs,
                                        size_t cur_bit,
                                        std::vector<boost::dynamic_bitset<>>& result) const {
    // TODO: optimize checking via counting bits
    if (IsFd(rhs) && lhs.is_proper_subset_of(cur_lhs)) {
        result.push_back(cur_lhs);
    }

    if (!HasChildren()) {
        return;
    }

    size_t next_lhs_bit = lhs.test(cur_bit) ? cur_bit : lhs.find_next(cur_bit);

    for (; cur_bit != num_attributes_ &&
           (next_lhs_bit == boost::dynamic_bitset<>::npos || cur_bit != next_lhs_bit + 1);
         ++cur_bit) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->GetSpecialsRecursive(lhs, cur_lhs, rhs, cur_bit + 1, result);
            cur_lhs.reset(cur_bit);
        }
    }
}

void FDTreeVertex::GetFdAndSpecialsRecursive(boost::dynamic_bitset<> const& lhs,
                                             boost::dynamic_bitset<>& cur_lhs, size_t rhs,
                                             size_t cur_bit,
                                             std::vector<boost::dynamic_bitset<>>& result) const {
    // TODO: optimize checking via counting bits
    if (IsFd(rhs) && lhs.is_subset_of(cur_lhs)) {
        result.push_back(cur_lhs);
    }

    if (!HasChildren()) {
        return;
    }

    size_t next_lhs_bit = lhs.test(cur_bit) ? cur_bit : lhs.find_next(cur_bit);

    for (; cur_bit != num_attributes_ &&
           (next_lhs_bit == boost::dynamic_bitset<>::npos || cur_bit != next_lhs_bit + 1);
         ++cur_bit) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->GetSpecialsRecursive(lhs, cur_lhs, rhs, cur_bit + 1, result);
            cur_lhs.reset(cur_bit);
        }
    }
}

bool FDTreeVertex::ContainsFdOrGeneralRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                                size_t cur_bit) const {
    if (IsFd(rhs)) {
        return true;
    }

    if (cur_bit == boost::dynamic_bitset<>::npos) {
        return false;
    }

    if (HasChildren() && ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs) &&
        children_[cur_bit]->ContainsFdOrGeneralRecursive(lhs, rhs, lhs.find_next(cur_bit))) {
        return true;
    }

    return ContainsFdOrGeneralRecursive(lhs, rhs, lhs.find_next(cur_bit));
}

bool FDTreeVertex::ContainsFdOrSpecialRecursive(boost::dynamic_bitset<> const& lhs, size_t rhs,
                                                size_t next_after_last_lhs_set_bit,
                                                size_t cur_bit) const {
    if (IsFd(rhs) && cur_bit >= next_after_last_lhs_set_bit) {
        return true;
    }

    if (cur_bit == num_attributes_) {
        return false;
    }

    if (HasChildren() && ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs) &&
        children_[cur_bit]->ContainsFdOrSpecialRecursive(lhs, rhs, next_after_last_lhs_set_bit,
                                                         cur_bit + 1)) {
        return true;
    }

    if (lhs.test(cur_bit)) {
        return false;
    }

    return ContainsFdOrSpecialRecursive(lhs, rhs, next_after_last_lhs_set_bit, cur_bit + 1);
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
            children_[current_lhs_attr].reset();
            children_[current_lhs_attr] = nullptr;
        }
    }

    if (IsLastNodeOf(rhs)) {
        contains_children_ = false;
        RemoveAttribute(rhs);
        return true;
    }
    return false;
}

void FDTreeVertex::RemoveGeneralsRecursive(boost::dynamic_bitset<> const& lhs,
                                           boost::dynamic_bitset<> cur_lhs, size_t rhs,
                                           size_t cur_bit) {
    // TODO: optimize checking via counting bits
    if (IsFd(rhs) && lhs != cur_lhs) {
        RemoveFd(rhs);
        RemoveAttribute(rhs);
    }

    if (!HasChildren()) {
        return;
    }

    for (; cur_bit != boost::dynamic_bitset<>::npos; cur_bit = lhs.find_next(cur_bit)) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->RemoveGeneralsRecursive(lhs, cur_lhs, rhs, lhs.find_next(cur_bit));
            cur_lhs.reset(cur_bit);

            if (!children_[cur_bit]->GetAttributes().any()) {
                children_[cur_bit].reset();
                children_[cur_bit] = nullptr;
            }
        }
    }

    if (IsLastNodeOf(rhs)) {
        contains_children_ = false;
        RemoveAttribute(rhs);
    }
}

void FDTreeVertex::RemoveSpecialsRecursive(boost::dynamic_bitset<> const& lhs,
                                           boost::dynamic_bitset<> cur_lhs, size_t rhs,
                                           size_t cur_bit) {
    // TODO: optimize checking via counting bits
    if (IsFd(rhs) && lhs.is_subset_of(cur_lhs)) {
        RemoveFd(rhs);
        RemoveAttribute(rhs);
    }

    if (!HasChildren()) {
        return;
    }

    size_t next_lhs_bit = lhs.test(cur_bit) ? cur_bit : lhs.find_next(cur_bit);

    for (; cur_bit != num_attributes_ &&
           (next_lhs_bit == boost::dynamic_bitset<>::npos || cur_bit != next_lhs_bit + 1);
         ++cur_bit) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->RemoveSpecialsRecursive(lhs, cur_lhs, rhs, cur_bit + 1);
            cur_lhs.reset(cur_bit);

            if (!children_[cur_bit]->GetAttributes().any()) {
                children_[cur_bit].reset();
                children_[cur_bit] = nullptr;
            }
        }
    }

    if (IsLastNodeOf(rhs)) {
        contains_children_ = false;
        RemoveAttribute(rhs);
    }
}

bool FDTreeVertex::IsLastNodeOf(size_t rhs) const noexcept {
    if (!HasChildren()) {
        return true;
    }
    return std::all_of(children_.cbegin(), children_.cend(), [rhs](auto const& child) {
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

    if (!contains_children_) {
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
