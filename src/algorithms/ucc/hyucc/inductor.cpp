#include "inductor.h"

namespace algos {

void Inductor::UpdateUCCTree(NonUCCList&& non_uccs) {
    unsigned const max_level = non_uccs.GetDepth();

    for (unsigned level = max_level; level != 0; --level) {
        for (auto const& non_ucc : non_uccs.GetLevel(level)) {
            SpecializeUCCTree(non_ucc);
        }
    }
}

void Inductor::SpecializeUCCTree(boost::dynamic_bitset<> const& non_ucc) {
    std::vector<boost::dynamic_bitset<>> invalid_uccs = tree_->GetUCCAndGeneralizations(non_ucc);

    for (auto& invalid_ucc : invalid_uccs) {
        tree_->Remove(invalid_ucc);
        for (size_t attr_num = tree_->GetNumAttributes(); attr_num > 0; --attr_num) {
            size_t attr_idx = attr_num - 1;

            if (!non_ucc.test(attr_idx)) {
                invalid_ucc.set(attr_idx);
                if (!tree_->FindUCCOrGeneralization(invalid_ucc)) {
                    tree_->AddUCC(invalid_ucc);
                }
                invalid_ucc.reset(attr_idx);
            }
        }
    }
}

}  // namespace algos
