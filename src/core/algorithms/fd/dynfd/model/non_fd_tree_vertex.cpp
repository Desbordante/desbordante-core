#include "non_fd_tree_vertex.h"

#include <algorithm>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dynfd {

bool NonFDTreeVertex::AddChild(size_t const pos) {
    if (children_.empty()) {
        children_.resize(num_attributes_);
    }

    if (!ContainsChildAt(pos)) {
        children_[pos] = std::make_shared<NonFDTreeVertex>(num_attributes_);
        children_count_++;
        return true;
    }

    return false;
}

void NonFDTreeVertex::GetLevelRecursive(unsigned const target_level, unsigned const cur_level,
                                        boost::dynamic_bitset<> lhs,
                                        std::vector<LhsPair>& vertices) {
    if (cur_level == target_level) {
        if (non_fds_.any()) {
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

void NonFDTreeVertex::GetNonFdAndSpecialsRecursive(
        boost::dynamic_bitset<> const& lhs, boost::dynamic_bitset<>& cur_lhs, size_t rhs,
        size_t cur_bit, std::vector<boost::dynamic_bitset<>>& result) const {
    // TODO: optimize checking via counting bits
    size_t next_lhs_bit;
    if (cur_bit < lhs.size()) {
        next_lhs_bit = lhs.test(cur_bit) ? cur_bit : lhs.find_next(cur_bit);
    } else {
        next_lhs_bit = boost::dynamic_bitset<>::npos;
    }

    if (IsNonFd(rhs) && next_lhs_bit == boost::dynamic_bitset<>::npos) {
        result.push_back(cur_lhs);
        return;
    }

    if (!HasChildren()) {
        return;
    }

    auto limit = next_lhs_bit == boost::dynamic_bitset<>::npos ? num_attributes_ - 1 : next_lhs_bit;

    for (; cur_bit <= limit; ++cur_bit) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            cur_lhs.set(cur_bit);
            children_[cur_bit]->GetNonFdAndSpecialsRecursive(lhs, cur_lhs, rhs, cur_bit + 1,
                                                             result);
            cur_lhs.reset(cur_bit);
        }
    }
}

bool NonFDTreeVertex::ContainsNonFdOrSpecialRecursive(boost::dynamic_bitset<> const& lhs,
                                                      size_t rhs, size_t cur_bit) const {
    size_t next_lhs_bit;
    if (cur_bit < lhs.size()) {
        next_lhs_bit = lhs.test(cur_bit) ? cur_bit : lhs.find_next(cur_bit);
    } else {
        next_lhs_bit = boost::dynamic_bitset<>::npos;
    }

    if (IsNonFd(rhs) && next_lhs_bit == boost::dynamic_bitset<>::npos) {
        return true;
    }

    if (!HasChildren()) {
        return false;
    }

    auto limit = next_lhs_bit == boost::dynamic_bitset<>::npos ? num_attributes_ - 1 : next_lhs_bit;

    for (; cur_bit <= limit; ++cur_bit) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs) &&
            children_[cur_bit]->ContainsNonFdOrSpecialRecursive(lhs, rhs, cur_bit + 1)) {
            return true;
        }
    }

    return false;
}

bool NonFDTreeVertex::RemoveRecursive(boost::dynamic_bitset<> const& lhs, size_t const rhs,
                                      size_t current_lhs_attr) {
    if (current_lhs_attr == boost::dynamic_bitset<>::npos) {
        RemoveNonFd(rhs);
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

void NonFDTreeVertex::RemoveGeneralsRecursive(boost::dynamic_bitset<> const& lhs, size_t const rhs,
                                              size_t cur_bit, bool is_generalized) {
    // TODO: optimize checking via counting bits
    if (IsNonFd(rhs) && is_generalized) {
        RemoveNonFd(rhs);
        RemoveAttribute(rhs);
    }

    if (!HasChildren()) {
        return;
    }

    for (; cur_bit != boost::dynamic_bitset<>::npos; cur_bit = lhs.find_next(cur_bit)) {
        if (ContainsChildAt(cur_bit) && children_[cur_bit]->IsAttribute(rhs)) {
            children_[cur_bit]->RemoveGeneralsRecursive(lhs, rhs, lhs.find_next(cur_bit),
                                                        is_generalized);

            if (children_[cur_bit]->GetAttributes().none()) {
                children_[cur_bit] = nullptr;
                children_count_--;
            }
        }
        is_generalized = true;
    }

    if (IsLastNodeOf(rhs)) {
        RemoveAttribute(rhs);
    }
}

bool NonFDTreeVertex::IsLastNodeOf(size_t rhs) const noexcept {
    if (!HasChildren()) {
        return true;
    }
    return !std::ranges::any_of(children_, [rhs](auto const& child) {
        return (child != nullptr) && child->IsAttribute(rhs);
    });
}

std::shared_ptr<NonFDTreeVertex> NonFDTreeVertex::GetChildIfExists(size_t const pos) const {
    if (children_.empty()) {
        return nullptr;
    }

    assert(pos < children_.size());
    return children_[pos];
}

void NonFDTreeVertex::FillNonFDs(std::vector<RawFD>& fds, boost::dynamic_bitset<>& lhs) const {
    for (size_t rhs = non_fds_.find_first(); rhs != boost::dynamic_bitset<>::npos;
         rhs = non_fds_.find_next(rhs)) {
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
        GetChild(i)->FillNonFDs(fds, lhs);
        lhs.reset(i);
    }
}

bool NonFDTreeVertex::IsNonFdViolatingPairHolds(size_t pos,
                                                std::shared_ptr<DynamicRelationData> relation_) {
    if (!IsNonFd(pos) || !violations_[pos]) {
        return false;
    }

    ViolatingRecordPair& violating_pair = violations_[pos];
    if (relation_->IsRowIndexValid(violating_pair->first) &&
        relation_->IsRowIndexValid(violating_pair->second)) {
        return true;
    } else {
        violating_pair = std::nullopt;
        return false;
    }
}

}  // namespace algos::dynfd
