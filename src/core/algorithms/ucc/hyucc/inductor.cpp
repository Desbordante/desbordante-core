#include "core/algorithms/ucc/hyucc/inductor.h"

namespace algos::hyucc {

void Inductor::UpdateUCCTree(NonUCCList&& non_uccs) {
    unsigned const max_level = non_uccs.GetDepth();

    for (unsigned level = max_level; level != 0; --level) {
        std::vector<model::RawUCC> cur_level = non_uccs.GetLevel(level);
        for (auto const& non_ucc : cur_level) {
            SpecializeUCCTree(non_ucc);
        }
    }
}

void Inductor::SpecializeUCCTree(model::RawUCC const& non_ucc) {
    std::vector<model::RawUCC> invalid_uccs = tree_->GetUCCAndGeneralizations(non_ucc);

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

}  // namespace algos::hyucc
