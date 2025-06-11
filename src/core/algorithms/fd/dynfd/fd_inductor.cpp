#include "fd_inductor.h"

void algos::dynfd::FDInductor::UpdateCovers(algos::hy::ColumnCombinationList const& agree_sets) {
    for (int level = agree_sets.GetDepth() - 1; level >= 0; --level) {
        for (auto const& lhs : agree_sets.GetLevel(level)) {
            auto rhss = lhs;
            rhss.flip();

            for (auto rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
                 rhs = rhss.find_next(rhs)) {
                DeduceDependencies({lhs, rhs});
            }
        }
    }
}

void algos::dynfd::FDInductor::DeduceDependencies(RawFD non_fd) {
    for (auto const& non_fd_lhs : positive_cover_tree_->GetFdAndGenerals(non_fd.lhs_, non_fd.rhs_)) {
        positive_cover_tree_->Remove(non_fd_lhs, non_fd.rhs_);

        boost::dynamic_bitset<> lhs = non_fd.lhs_;
        lhs.flip();
        lhs.reset(non_fd.rhs_);
        for (size_t lhs_attr = lhs.find_first();
             lhs_attr != boost::dynamic_bitset<>::npos;
             lhs_attr = lhs.find_next(lhs_attr)) {
            boost::dynamic_bitset<> new_lhs = non_fd.lhs_;
            new_lhs.set(lhs_attr);

            if (!positive_cover_tree_->ContainsFdOrGeneral(new_lhs, non_fd.rhs_)) {
                positive_cover_tree_->AddFD(new_lhs, non_fd.rhs_);
            }
        }
    }

    if (!negative_cover_tree_->ContainsNonFdOrSpecial(non_fd.lhs_, non_fd.rhs_)) {
        negative_cover_tree_->RemoveGenerals(non_fd.lhs_, non_fd.rhs_);
        negative_cover_tree_->AddNonFD(non_fd.lhs_, non_fd.rhs_, std::nullopt);
    }
}
