#include "fd_inductor.h"

void algos::dynfd::FDInductor::UpdateCovers(algos::hy::ColumnCombinationList const& agree_sets) {
    for (int level = agree_sets.GetDepth(); level > 0; --level) {
        for (auto const& lhs : agree_sets.GetLevel(level)) {
            for (size_t rhs = 0; rhs < lhs.size(); ++rhs) {
                if (!lhs[rhs]) {
                    DeduceDependencies(lhs, rhs);
                }
            }
        }
    }
}

void algos::dynfd::FDInductor::DeduceDependencies(boost::dynamic_bitset<> const& lhs, size_t rhs) {
    for (auto const& fd_lhs : positive_cover_tree_->GetFdAndGenerals(lhs, rhs)) {
        positive_cover_tree_->Remove(fd_lhs, rhs);

        for (size_t lhs_attr = 0; lhs_attr < lhs.size(); ++lhs_attr) {
            if (!lhs[lhs_attr] && lhs_attr != rhs) {
                boost::dynamic_bitset<> new_lhs = fd_lhs;
                new_lhs.set(lhs_attr);

                if (!positive_cover_tree_->ContainsFdOrGeneral(new_lhs, rhs)) {
                    positive_cover_tree_->AddFD(new_lhs, rhs);
                }
            }
        }
    }

    if (!negative_cover_tree_->ContainsNonFdOrSpecial(lhs, rhs)) {
        negative_cover_tree_->RemoveGenerals(lhs, rhs);
        negative_cover_tree_->AddNonFD(lhs, rhs, std::nullopt);
    }
}
