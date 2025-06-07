#include "non_fd_inductor.h"

namespace algos::dynfd {
void NonFDInductor::FindFds(std::vector<RawFD> const& valid_fds) {
    int sample_size = std::ceil(static_cast<float>(valid_fds.size()) / 10);
    for (int index = 0; index < sample_size; ++index) {
        Dfs(valid_fds[index]);
    }
}

void NonFDInductor::Dfs(RawFD fd, int next_lhs_attr) {
    auto removed_lhs_attr = next_lhs_attr == -1
        ? fd.lhs_.find_first()
        : fd.lhs_.find_next(next_lhs_attr);
    for (; removed_lhs_attr != boost::dynamic_bitset<>::npos;
         removed_lhs_attr = fd.lhs_.find_next(removed_lhs_attr)) {
        boost::dynamic_bitset<> new_lhs = fd.lhs_;
        new_lhs.reset(removed_lhs_attr);
        RawFD newFd{new_lhs, fd.rhs_};

        if (positive_cover_tree_->ContainsFdOrGeneral(new_lhs, fd.rhs_) || 
            validator_->IsNonFdValidated(newFd)) {
            Dfs(newFd, removed_lhs_attr + 1);
            return;
        }
    }

    DeduceNonFds(fd);
}

void NonFDInductor::DeduceNonFds(RawFD fd) {
    auto valid_lhs = negative_cover_tree_->GetNonFdAndSpecials(fd.lhs_, fd.rhs_);
    for (const auto &non_fd_lhs : valid_lhs) {
        negative_cover_tree_->Remove(non_fd_lhs, fd.rhs_);

        for (size_t removed_lhs_attribute = fd.lhs_.find_first();
             removed_lhs_attribute != boost::dynamic_bitset<>::npos;
             removed_lhs_attribute = fd.lhs_.find_next(removed_lhs_attribute)) {
            boost::dynamic_bitset<> new_lhs = fd.lhs_;
            new_lhs.reset(removed_lhs_attribute);

            if (!negative_cover_tree_->ContainsNonFdOrSpecial(new_lhs, fd.rhs_)) {
                negative_cover_tree_->AddNonFD(new_lhs, fd.rhs_, std::nullopt);
            }
        }
    }

    if (!positive_cover_tree_->ContainsFdOrGeneral(fd.lhs_, fd.rhs_)) {
        positive_cover_tree_->RemoveSpecials(fd.lhs_, fd.rhs_);
        positive_cover_tree_->AddFD(fd.lhs_, fd.rhs_);
    }
}
} // namespace algos::dynfd
