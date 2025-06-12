#include "non_fd_inductor.h"

namespace algos::dynfd {
void NonFDInductor::FindFds(std::vector<RawFD> const& valid_fds) {
    for (int index = 0; index < valid_fds.size(); index += 10) {
        Dfs(valid_fds[index], valid_fds[index].lhs_.find_first());
    }
}

void NonFDInductor::Dfs(RawFD fd, int next_lhs_attr) {
    for (auto removed_lhs_attr = next_lhs_attr;
         removed_lhs_attr != boost::dynamic_bitset<>::npos;
         removed_lhs_attr = fd.lhs_.find_next(removed_lhs_attr)) {
        auto new_lhs = fd.lhs_;
        new_lhs.reset(removed_lhs_attr);
        RawFD newFd{new_lhs, fd.rhs_};

        if (positive_cover_tree_->ContainsFdOrGeneral(new_lhs, fd.rhs_) || 
            validator_->IsNonFdValidated(newFd)) {
            Dfs(newFd, newFd.lhs_.find_next(removed_lhs_attr));
            return;
        }
    }

    DeduceNonFds(fd);
}

void NonFDInductor::DeduceNonFds(RawFD fd) {
    auto rhs = fd.rhs_;
    for (auto const& non_fd_lhs : negative_cover_tree_->GetNonFdAndSpecials(fd.lhs_, fd.rhs_)) {
        negative_cover_tree_->Remove(non_fd_lhs, rhs);

        for (size_t removed_lhs_attr = fd.lhs_.find_first();
             removed_lhs_attr != boost::dynamic_bitset<>::npos;
             removed_lhs_attr = fd.lhs_.find_next(removed_lhs_attr)) {
            boost::dynamic_bitset<> new_lhs = non_fd_lhs;
            new_lhs.reset(removed_lhs_attr);

            if (!negative_cover_tree_->ContainsNonFdOrSpecial(new_lhs, rhs)) {
                negative_cover_tree_->AddNonFD(new_lhs, rhs, std::nullopt);
            }
        }
    }

    if (!positive_cover_tree_->ContainsFdOrGeneral(fd.lhs_, fd.rhs_)) {
        positive_cover_tree_->RemoveSpecials(fd.lhs_, fd.rhs_);
        positive_cover_tree_->AddFD(fd.lhs_, fd.rhs_);
    }
}
} // namespace algos::dynfd
