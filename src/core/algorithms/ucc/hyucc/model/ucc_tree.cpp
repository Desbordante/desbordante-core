#include "core/algorithms/ucc/hyucc/model/ucc_tree.h"

namespace algos::hyucc {

UCCTreeVertex* UCCTree::AddUCC(boost::dynamic_bitset<> const& ucc, bool* is_new_out) {
    UCCTreeVertex* cur_node = root_.get();

    assert(ucc.any());
    for (size_t attr = ucc.find_first(); attr != boost::dynamic_bitset<>::npos;
         attr = ucc.find_next(attr)) {
        bool is_new = cur_node->AddChild(attr);
        if (is_new_out != nullptr) {
            *is_new_out = is_new;
        }
        cur_node = cur_node->GetChild(attr);
    }

    cur_node->is_ucc_ = true;
    return cur_node;
}

UCCTreeVertex* UCCTree::AddUCCGetIfNew(boost::dynamic_bitset<> const& ucc) {
    bool is_new;
    UCCTreeVertex* added = AddUCC(ucc, &is_new);
    if (is_new) {
        return added;
    } else {
        return nullptr;
    }
}

std::vector<boost::dynamic_bitset<>> UCCTree::FillUCCs() const {
    std::vector<boost::dynamic_bitset<>> result;
    boost::dynamic_bitset<> ucc(root_->GetNumAttributes());
    root_->FillUCCsRecursive(result, std::move(ucc));
    return result;
}

}  // namespace algos::hyucc
