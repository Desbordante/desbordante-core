#include "fd_tree.h"

namespace algos::hyfd::fd_tree {

std::shared_ptr<FDTreeVertex> FDTree::AddFD(boost::dynamic_bitset<> const& lhs, size_t rhs) {
    std::shared_ptr<FDTreeVertex> cur_node = root_;
    cur_node->SetAttribute(rhs);

    bool is_new = false;
    for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
         bit = lhs.find_next(bit)) {
        is_new = cur_node->AddChild(bit);
        cur_node = cur_node->GetChild(bit);
        cur_node->SetAttribute(rhs);
    }
    cur_node->SetFd(rhs);
    if (is_new) {
        return cur_node;
    }
    return nullptr;
}

bool FDTree::ContainsFD(boost::dynamic_bitset<> const& lhs, size_t rhs) {
    std::shared_ptr<FDTreeVertex> cur_node = root_;

    for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
         bit = lhs.find_next(bit)) {
        if (!cur_node->HasChildren() || !cur_node->ContainsChildAt(bit)) {
            return false;
        }

        cur_node = cur_node->GetChild(bit);
    }

    return cur_node->IsFd(rhs);
}

std::vector<boost::dynamic_bitset<>> FDTree::GetFdAndGenerals(boost::dynamic_bitset<> const& lhs,
                                                              size_t rhs) const {
    assert(lhs.count() != 0);

    std::vector<boost::dynamic_bitset<>> result;
    boost::dynamic_bitset<> const empty_lhs(GetNumAttributes());
    size_t const starting_bit = lhs.find_first();

    root_->GetFdAndGeneralsRecursive(lhs, empty_lhs, rhs, starting_bit, result);

    return result;
}

std::vector<LhsPair> FDTree::GetLevel(unsigned target_level) {
    boost::dynamic_bitset<> const empty_lhs(GetNumAttributes());

    std::vector<LhsPair> vertices;
    root_->GetLevelRecursive(target_level, 0, empty_lhs, vertices);
    return vertices;
}

}  // namespace algos::hyfd::fd_tree